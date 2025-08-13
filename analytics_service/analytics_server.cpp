#include "analytics_server.h"

size_t line_counter=0;
std::atomic<bool> shutdown_requested{false};
const double EPS = 1e-7;

double MetricAggregator::MetricData::compute_stddev(){
    size_t n = values.size();
    if(n == 0) return 0;
    double mean = sum/n;
    double variance = sum_squares / n - mean*mean;
    if(variance < 0.0) variance=0.0;
    return std::sqrt(variance);
}

void MetricAggregator::update_metrics_for_device(const std::string& device_id, double temp_value, 
    double humidity_value, int64_t timestamp, iot::metrics::PrometheusMetrics& metrics){
    auto& data = metrics_[device_id];

    //update temp and humidity
    update_metric(data.temperature,temp_value, timestamp);
    update_metric(data.humidity, humidity_value, timestamp);

    size_t window_temp  = data.temperature.values.size();
    size_t window_humid = data.humidity.values.size();

    double avg_temp     = window_temp ? data.temperature.sum / window_temp : 0.0;
    double min_temp     = window_temp ? data.temperature.min               : 0.0;
    double max_temp     = window_temp ? data.temperature.max               : 0.0;
    double stddev_temp  = window_temp ? data.temperature.compute_stddev()  : 0.0;  
    
    double avg_humid    = window_humid ? data.humidity.sum / window_humid : 0.0;
    double min_humid    = window_humid ? data.humidity.min                : 0.0;
    double max_humid    = window_humid ? data.humidity.max                : 0.0;
    double stddev_humid = window_humid ? data.humidity.compute_stddev()   : 0.0;

    metrics.publishMetrics(device_id,
        avg_temp, min_temp, max_temp, stddev_temp,
        avg_humid, min_humid, max_humid, stddev_humid,
        window_temp, window_humid
    );
}

void MetricAggregator::add_values(const std::vector<SensorData>& batch, iot::metrics::PrometheusMetrics& metrics){
    std::unique_lock lock(mutex_);
    for(const auto& data : batch){
        update_metrics_for_device(data.device_id(), data.temperature(),data.humidity(),
                                    data.timestamp().seconds(),metrics);
    }
}

void MetricAggregator::add_value(const std::string& device_id, double temp_value, double humidity_value, 
                                int64_t timestamp, iot::metrics::PrometheusMetrics& metrics){
    std::unique_lock lock(mutex_);
    update_metrics_for_device(device_id, temp_value, humidity_value, timestamp, metrics);
}

void MetricAggregator::update_metric(MetricData& metric, double value, int64_t timestamp){
    metric.values.emplace_back(value,timestamp);
    metric.sum += value;
    metric.sum_squares += (value * value);

    if(value < metric.min) metric.min = value;
    if(value > metric.max) metric.max = value;

    while(!metric.values.empty() && 
    timestamp - metric.values.front().second > iot::config::Thresholds::aggregation_window_sec){
        metric.sum -= metric.values.front().first;
        metric.sum_squares -= (metric.values.front().first * metric.values.front().first);
        metric.values.pop_front();
    }
}

std::optional<MetricAggregator::Metrics> MetricAggregator::get_metrics(const std::string& device_id) const {
    std::shared_lock lock(mutex_);
    auto it = metrics_.find(device_id);
    if(it == metrics_.end()) return std::nullopt;
    return it->second;
}

void AlertManager::check_thresholds(const SensorData& data, iot::metrics::PrometheusMetrics& metrics){
    bool alert = false;
    Alert alert_msg;
    alert_msg.set_device_id(data.device_id());
    *alert_msg.mutable_timestamp() = data.timestamp();

    if(data.temperature() > iot::config::Thresholds::temperature){
        alert = true;
        alert_msg.set_metric_type("temperature");
        alert_msg.set_current_value(data.temperature());
        alert_msg.set_severity("critical");
        metrics.alerts_triggered_temperature().Increment();
    }

    if(data.humidity() > iot::config::Thresholds::humidity){
        alert = true;
        alert_msg.set_metric_type("humidity");
        alert_msg.set_current_value(data.humidity());
        alert_msg.set_severity("warning");
        metrics.alerts_triggered_humidity().Increment();
    }

    if(alert){
        std::lock_guard lock(alerts_mutex_);
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

    StatResponse response;
    while(!context->IsCancelled() && !shutdown_requested.load()){
        
        auto metrics = aggregator_->get_metrics(request->device_id(0));
        auto type = request->aggregation();

        if(metrics){
            auto* stat = response.mutable_metrics();
            if(type == iot::v1::StatRequest_AggregationType_AVG){
                if(!metrics->temperature.values.empty()){
                    (*stat)["temperature_avg"] = metrics->temperature.sum / metrics->temperature.values.size();
                }
                if(!metrics->humidity.values.empty()){
                    (*stat)["humidity_avg"] = metrics->humidity.sum / metrics->humidity.values.size();
                }
            }else if(type == iot::v1::StatRequest_AggregationType_MIN){
                if(!metrics->temperature.values.empty()){
                    (*stat)["temperature_min"] = metrics->temperature.min;
                }
                if(!metrics->humidity.values.empty()){
                    (*stat)["humidity_min"] = metrics->humidity.min;
                }
            }else if(type == iot::v1::StatRequest_AggregationType_MAX){
                if(!metrics->temperature.values.empty()){
                    (*stat)["temperature_max"] = metrics->temperature.max;
                }    
                if(!metrics->humidity.values.empty()){
                    (*stat)["humidity_max"] = metrics->humidity.max;
                }
            }else if(type == iot::v1::StatRequest_AggregationType_STDDEV){
                if(!metrics->temperature.values.empty()){
                    (*stat)["temperature_stddev"] = metrics->temperature.compute_stddev();
                }    
                if(!metrics->humidity.values.empty()){
                    (*stat)["humidity_stddev"] = metrics->humidity.compute_stddev();
                }
            }else{
                std::string ans = "Error, type not supported";
                response.mutable_metrics()->insert({ans, -1});
            }

            auto now = std::chrono::system_clock::now();
            *response.mutable_timestamp() = google::protobuf::util::TimeUtil::TimeTToTimestamp(
                std::chrono::system_clock::to_time_t(now));
            writer->Write(response);
        }else{
            std::string ans = "Error, device_id: " + request->device_id(0) + " not found";
            response.mutable_metrics()->insert({ans, -1});

            auto now = std::chrono::system_clock::now();
            *response.mutable_timestamp() = google::protobuf::util::TimeUtil::TimeTToTimestamp(
                std::chrono::system_clock::to_time_t(now));
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
            if(matches_subscription(alert, *sub)){
                writer->Write(alert);
            }
        }
        std::this_thread::sleep_for(500ms);
    }
    return Status::OK;
}

bool AnalyticsServiceImpl::matches_subscription(const Alert& alert, const AlertSubscription& sub){
    // Проверка device_ids
    if (!sub.device_ids().empty() && 
    std::find(sub.device_ids().begin(), 
            sub.device_ids().end(),
            alert.device_id()) == sub.device_ids().end()) {
    return false;
    }

    // Проверка типов метрик
    if (!sub.metric_types().empty() &&
        std::find(sub.metric_types().begin(),
                sub.metric_types().end(),
                alert.metric_type()) == sub.metric_types().end()) {
        return false;
    }

    // Проверка уровней серьезности
    if (!sub.severity_levels().empty() &&
        std::find(sub.severity_levels().begin(),
                sub.severity_levels().end(),
                alert.severity()) == sub.severity_levels().end()) {
        return false;
    }

    return true;
}

void AnalyticsServer::Run(){
    setup_signal_handlers();

    start_grpc_server();

    std::this_thread::sleep_for(500ms); 

    start_redis_consumer();
    wait_for_shutdown();
    cleanup();
}

void AnalyticsServer::setup_signal_handlers(){
    std::signal(SIGINT, [](int){shutdown_requested.store(true); });
    std::signal(SIGTERM, [](int){shutdown_requested.store(true);} );
    std::signal(SIGSEGV, [](int) {
        std::cerr << "SEGFAULT detected!" << std::endl;
        std::abort();
    });
}

void AnalyticsServer::redis_consumer(std::shared_ptr<MetricAggregator> aggregator,
                                     std::shared_ptr<AlertManager> alert_manager,
                                     iot::metrics::PrometheusMetrics &metrics,
                                     std::stop_token st){
    auto redis = std::make_shared<sw::redis::Redis>(iot::config::NetworkingSettings::redis_host());
    std::string stream_name = iot::config::StreamSettings::stream_name;
    std::string group_name = iot::config::StreamSettings::consumer_group;
    std::string consumer_name = iot::config::StreamSettings::consumer_name();

    try{
        redis->xgroup_create(stream_name,group_name,"0-0", true);
    }catch(const sw::redis::Error& e){
        if(e.what() != std::string{"BUSYGROUP Consumer Group name already exists"}){
            logger_->error("Error create group on alaytics server: {}", e.what());
            throw;
        }
        
    }

    std::vector<std::string> ids_to_ack;
    ids_to_ack.reserve(iot::config::StreamSettings::max_batch_size);

    while (!st.stop_requested()) {
        try {
            auto start = std::chrono::steady_clock::now();
            iot::parser::RedisStreamParser::StreamResult result;

            redis->xreadgroup(group_name,
                        consumer_name,
                        stream_name,
                        ">",
                        iot::config::StreamSettings::read_timeout,
                        iot::config::StreamSettings::max_batch_size,
                        false, // noask = false (требуем явного ACK)
                        std::inserter(result, result.end()));

            if (st.stop_requested()) break;

            //обработка батча
            if (auto stream_it = result.find(stream_name); stream_it != result.end()) {
                std::vector<SensorData> batch;
                batch.reserve(stream_it->second.size());

                for (const auto entry : stream_it->second) {
                    try {
                        SensorData data = iot::parser::RedisStreamParser::parse_sensor_data(entry.first,entry.second);
                        batch.push_back(data);
                        ids_to_ack.push_back(entry.first);
                    } catch (const iot::parser::StreamParserException &e) {
                        //std::cerr << "Pasre error: " << e.what() << '\n';
                        logger_->error("Pasre error: ", e.what());
                        metrics.parse_error().Increment();
                    }
                }

                if(!batch.empty()){
                    aggregator->add_values(batch,metrics);
                    for(const auto& data : batch){\
                        //измерение сквозной задержки
                        //считаем тут, тк нужно время между отправкой данных и временем их получения в системе
                        auto now = std::chrono::system_clock::now();
                        auto sensor_time = google::protobuf::util::TimeUtil::TimestampToTimeT(data.timestamp());
                        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - std::chrono::system_clock::from_time_t(sensor_time)).count() / 1000.0;
                        metrics.end_to_end_lanency().Observe(latency);

                        //инкремент счетчика сообщений для устройства
                        metrics.message_processed_per_device().Add({{"device_id", data.device_id()}}).Increment();

                        alert_manager->check_thresholds(data,metrics);
                    }
                }

                //подтвержднеие обработки
                ack_messages(*redis,stream_name,group_name,ids_to_ack,metrics);
            }
            update_metrics(metrics,start,result.size());
            
        } catch (const std::exception &e) {
            logger_->error("Redis error: ", e.what());
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

void AnalyticsServer::process_entry(const std::pair<std::string, iot::parser::RedisStreamParser::FieldMap>& entry,
    MetricAggregator& aggregator, AlertManager& alert_manager, iot::metrics::PrometheusMetrics& metrics){

    SensorData data = iot::parser::RedisStreamParser::parse_sensor_data(entry.first, entry.second);

    aggregator.add_value(
        data.device_id(),
        data.temperature(),
        data.humidity(),
        data.timestamp().seconds(),
        metrics);

    alert_manager.check_thresholds(data, metrics);
}

void AnalyticsServer::ack_messages(sw::redis::Redis& redis, const std::string& stream,
        const std::string& group, std::vector<std::string>& ids, 
        iot::metrics::PrometheusMetrics& metrics){
    if(ids.empty()) return;

    try{
        redis.xack(stream,group,ids.begin(),ids.end());
        metrics.message_acked().Increment(ids.size());
    }catch(const std::exception& e){
        logger_->error("XACK error: ", e.what());
        metrics.ack_error().Increment(ids.size());
    }
    ids.clear();
}

void AnalyticsServer::update_metrics(iot::metrics::PrometheusMetrics& metrics,
    std::chrono::steady_clock::time_point start, size_t precessed_count){
        metrics.message_processed().Increment(precessed_count);
        auto end = std::chrono::steady_clock::now();
        metrics.processed_time().Observe(
            std::chrono::duration(end-start).count());
    }


void AnalyticsServer::start_grpc_server(){
    service_ = std::make_unique<AnalyticsServiceImpl>(aggregator_,alert_manager_);
    ServerBuilder builder;
    std::string server_address = iot::config::NetworkingSettings::analytics_service_address();
    builder.AddListeningPort(server_address,grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    server_ = builder.BuildAndStart();

    // Критическая проверка
    if (!server_) {
        logger_->error("Failed to start gRPC server!");
        exit(EXIT_FAILURE);
    }

    std::cout << "Analytics service listened on port " << server_address << " and name: " << iot::config::StreamSettings::consumer_name() <<'\n';
    logger_->info("Analytics service initialized");
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