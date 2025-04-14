#include "analytics_server.h"

size_t line_counter=0;
std::atomic<bool> shutdown_requested{false};

void MetricAggregator::add_value(const std::string& device_id, double value, int64_t timespamp,
    iot::metrics::PrometheusMetrics& metrics){
    std::unique_lock lock(mutex_);
    auto& data = metrics_[device_id];
    data.values.emplace_back(value,timespamp);
    data.sum += value;

    if(value < data.min) data.min = value;
    if(value > data.max) data.max = value;

    while(!data.values.empty() && 
    timespamp - data.values.front().second > iot::config::Thresholds::aggregation_window_sec){
        data.sum -= data.values.front().first;
        data.values.pop_front();
    }
    metrics.aggregation_window().Set(data.values.size());
}

std::optional<MetricAggregator::Metrics> MetricAggregator::get_metrics(const std::string& device_id) const {
    std::shared_lock lock(mutex_);
    auto it = metrics_.find(device_id);
    if(it == metrics_.end()) return std::nullopt;
    return it->second;
}

void AlertManager::check_thresholds(const SensorData& data, 
    iot::metrics::PrometheusMetrics& metrics){
    bool alert = false;
    Alert alert_msg;
    alert_msg.set_device_id(data.device_id());
    *alert_msg.mutable_timestamp() = data.timestamp();

    if(data.temperature() > iot::config::Thresholds::temperature){
    alert = true;
    alert_msg.set_metric_type("temperature");
    alert_msg.set_current_value(data.temperature());
    alert_msg.set_severity("critical");
    }

    if(data.humidity() > iot::config::Thresholds::humidity){
    alert = true;
    alert_msg.set_metric_type("humidity");
    alert_msg.set_current_value(data.humidity());
    alert_msg.set_severity("warning");
    }

    if(alert){
    std::lock_guard lock(alerts_mutex_);
    metrics.alerts_triggered().Increment();
    alerts_queue_.push_back(alert_msg);
    }
}

std::vector<Alert> AlertManager::get_alerts(){
    std::lock_guard lock(alerts_mutex_);
    std::vector<Alert> result;
    std::swap(result, alerts_queue_);
    return result;
}

Status AnalyticsServiceImpl::GetRealtimeStats(ServerContext* context, 
    const StatRequest* request, ServerWriter<StatResponse>* writer) {
    while(!context->IsCancelled() && !shutdown_requested.load()){
        StatResponse response;
        auto metrics = aggregator_->get_metrics(request->device_id(0));

        if(metrics){
            auto* stat = response.mutable_metrics();
            (*stat)["temperature_avg"] = metrics->sum / metrics->values.size();
            (*stat)["temperature_min"] = metrics->min;
            (*stat)["temperature_max"] = metrics->max;
            writer->Write(response);
        }
        std::this_thread::sleep_for(1s);
    }
    return Status::OK;
}

Status AnalyticsServiceImpl::SubscribeToAlerts(ServerContext* context, 
    const AlertSubscription* sub, ServerWriter<Alert>* writer) {
    while(!context->IsCancelled() && !shutdown_requested.load()){
        auto alerts = alert_manager_->get_alerts();
        for(const auto& alert : alerts){
            if(mathes_subscription(alert, *sub)){
                writer->Write(alert);
            }
        }
        std::this_thread::sleep_for(500ms);
    }
    return Status::OK;
}

bool AnalyticsServiceImpl::mathes_subscription(const Alert& alert, const AlertSubscription& sub){
    // // Проверка device_ids
    // if (!sub.device_ids().empty() && 
    // std::find(sub.device_ids().begin(), 
    //         sub.device_ids().end(),
    //         alert.device_id()) == sub.device_ids().end()) {
    // return false;
    // }

    // // Проверка типов метрик
    // if (!sub.metric_types().empty() &&
    //     std::find(sub.metric_types().begin(),
    //             sub.metric_types().end(),
    //             alert.metric_type()) == sub.metric_types().end()) {
    //     return false;
    // }

    // // Проверка уровней серьезности
    // if (!sub.severity_levels().empty() &&
    //     std::find(sub.severity_levels().begin(),
    //             sub.severity_levels().end(),
    //             alert.severity()) == sub.severity_levels().end()) {
    //     return false;
    // }

    return true;
}

void AnalyticsServer::Run(){
    setup_signal_handlers();
    start_redis_consumer();
    start_grpc_server();
    wait_for_shutdown();
    cleanup();
}

void AnalyticsServer::setup_signal_handlers(){
    std::signal(SIGINT, [](int){shutdown_requested.store(true); });
    std::signal(SIGTERM, [](int){shutdown_requested.store(true);} );
}

void AnalyticsServer::redis_consumer(std::shared_ptr<MetricAggregator> aggregator,
                                     std::shared_ptr<AlertManager> alert_manager,
                                     iot::metrics::PrometheusMetrics &metrics,
                                     std::stop_token st){
    auto redis = std::make_shared<sw::redis::Redis>(iot::config::NetworkingSettings::redis_host());
    std::string last_id = "0-0";

    while (!st.stop_requested()) {
        try {
            auto start = std::chrono::steady_clock::now();

            iot::parser::RedisStreamParser::StreamResult result;
            std::string stream_name = iot::config::StreamSettings::stream_name;

            redis->xread(stream_name,
                         last_id,
                         iot::config::StreamSettings::read_timeout,
                         iot::config::StreamSettings::max_batch_size,
                         std::inserter(result, result.end()));

            if (st.stop_requested()) break;

            if (auto stream_it = result.find(stream_name); stream_it != result.end()) {
                for (const auto entry : stream_it->second) {
                    try {
                        SensorData data = iot::parser::RedisStreamParser::parse_sensor_data(entry.first, entry.second);

                        std::cout << line_counter++ << " : " << data.device_id() << '\n';

                        aggregator->add_value(
                            data.device_id(),
                            data.temperature(),
                            data.timestamp().seconds(),
                            metrics);

                        alert_manager->check_thresholds(data, metrics);

                        last_id = entry.first;
                    } catch (const iot::parser::StreamParserException &e) {
                        std::cerr << "Pasre error: " << e.what() << '\n';
                    }
                }
            }
            metrics.message_processed().Increment();
            auto end = std::chrono::steady_clock::now();
            metrics.processed_time().Observe(
                std::chrono::duration<double>(end - start).count());
        } catch (const std::exception &e) {
            std::cerr << "Redis error: " << e.what() << '\n';
            metrics.parse_error().Increment();
            std::this_thread::sleep_for(1s);
        }
    }
}

void AnalyticsServer::start_redis_consumer(){
    redis_thread_ = std::jthread([this](std::stop_token st){
        redis_consumer(aggregator_, alert_manager_, metrics_, st);
    });
}

void AnalyticsServer::start_grpc_server(){
    AnalyticsServiceImpl service(aggregator_,alert_manager_);
    ServerBuilder builder;
    std::string server_address = iot::config::NetworkingSettings::analytics_service_address();
    builder.AddListeningPort(server_address,grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server_ = builder.BuildAndStart();
    std::cout << "Analytics service listened on port " << server_address << '\n';
}

void AnalyticsServer::wait_for_shutdown(){
    while(!shutdown_requested.load()){
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "Shutdown signal detected. Shutting down Analysis Server gracefully...\n";
    server_->Shutdown();
    server_->Wait();
}

void AnalyticsServer::cleanup(){
    redis_thread_.request_stop();
    if(redis_thread_.joinable()){
        redis_thread_.join();
    }
}