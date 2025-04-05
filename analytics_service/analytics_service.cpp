#include <iostream>
#include <sw/redis++/redis++.h>
#include <grpcpp/grpcpp.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include "memory"
#include <atomic>
#include <thread>
#include <string>
#include <csignal>
#include <chrono>
#include <cmath>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>
#include <csignal>
#include <optional>
#include "common.pb.h"
#include "analytics_service.grpc.pb.h"
#include <string_view>
#include <iterator>
#include "redis_stream_parser.h"

using namespace std::chrono_literals;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using iot::v1::AnalyticsService;
using iot::v1::StatRequest;
using iot::v1::StatResponse;
using iot::v1::AlertSubscription;
using iot::v1::Alert;
using iot::v1::SensorData;

size_t line_counter=0;

namespace supp_var{
	const std::string REDIS_SERV_ADRESS = "tcp://localhost:6379";
	const std::string ANALYS_SERV_ADDRESS = "0.0.0.0:50052";
    const std::string PROMETHEUS_ADDRESS = "0.0.0.0:8081";
    //const std::string CONSUMER_GROUP = "analystics_grp";
    const std::string STREAM_NAME = "telemetry_stream";
    const std::string ALERTS_STREAM = "alerts_stream";
    constexpr auto READ_TIMEOUT = std::chrono::milliseconds(100);
    constexpr size_t MAX_BATCH_SIZE = 10;

    constexpr size_t AGGREGGATION_WINDOW_SEC = 30; //30 точек для скользящего среднего
    constexpr double TEMP_THRESHOLD = 40.0; //5°C отклонение
    constexpr double HUMIDITY_THRESHOLD = 80.0; //20% отклонение
}




std::atomic<bool> shutdown_requested{false};

class MetricAggregator {
public:
    struct Metrics{
        std::deque<std::pair<double,int64_t>> values;
        double sum = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();
    };

    void add_value(const std::string& device_id, double value, int64_t timespamp){
        std::unique_lock lock(mutex_);
        auto& data = metrics_[device_id];
        data.values.emplace_back(value,timespamp);
        data.sum += value;

        if(value < data.min) data.min = value;
        if(value > data.max) data.max = value;

        while(!data.values.empty() && 
            timespamp - data.values.front().second > supp_var::AGGREGGATION_WINDOW_SEC){
                data.sum -= data.values.front().first;
                data.values.pop_front();
            }
    }

    std::optional<Metrics> get_metrics(const std::string& device_id) const {
        std::shared_lock lock(mutex_);
        auto it = metrics_.find(device_id);
        if(it == metrics_.end()) return std::nullopt;
        return it->second;
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Metrics> metrics_;
};

class AlertManager {
public:
    void check_thresholds(const SensorData& data){
        bool alert = false;
        Alert alert_msg;
        alert_msg.set_device_id(data.device_id());
        *alert_msg.mutable_timestamp() = data.timestamp();

        if(data.temperature() > supp_var::TEMP_THRESHOLD){
            alert = true;
            alert_msg.set_metric_type("temperature");
            alert_msg.set_current_value(data.temperature());
            alert_msg.set_severity("critical");
        }

        if(data.humidity() > supp_var::HUMIDITY_THRESHOLD){
            alert = true;
            alert_msg.set_metric_type("humidity");
            alert_msg.set_current_value(data.humidity());
            alert_msg.set_severity("warning");
        }

        if(alert){
            std::lock_guard lock(alerts_mutex_);
            alerts_queue_.push_back(alert_msg);
        }
    }

    std::vector<Alert> get_alerts(){
        std::lock_guard lock(alerts_mutex_);
        std::vector<Alert> result;
        std::swap(result, alerts_queue_);
        return result;
    }

private:
    mutable std::mutex alerts_mutex_;
    std::vector<Alert> alerts_queue_;
};

class AnalyticsServiceImpl : public AnalyticsService::Service{
public:
    AnalyticsServiceImpl(std::shared_ptr<MetricAggregator> aggregator,
                        std::shared_ptr<AlertManager> alert_manager)
        : aggregator_(aggregator), alert_manager_(alert_manager){}

    Status GetRealtimeStats(ServerContext* context, const StatRequest* request,
                    ServerWriter<StatResponse>* writer) override {
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


    Status SubscribeToAlerts(ServerContext* context, const AlertSubscription* sub,
                            ServerWriter<Alert>* writer) override {
        while(!context->IsCancelled() && shutdown_requested.load()){
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


private:
    bool mathes_subscription(const Alert& alert, const AlertSubscription& sub){
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

    std::shared_ptr<MetricAggregator> aggregator_;
    std::shared_ptr<AlertManager> alert_manager_;
};

void redis_consumer(std::shared_ptr<MetricAggregator> aggregator,
                    std::shared_ptr<AlertManager> alert_manager) {
    auto redis = std::make_shared<sw::redis::Redis>(supp_var::REDIS_SERV_ADRESS);
    std::string last_id = "0-0";

    while (!shutdown_requested.load()) {
        try {
            iot::parser::RedisStreamParser::StreamResult result;

            redis->xread(supp_var::STREAM_NAME,
                         last_id,
                         supp_var::READ_TIMEOUT,
                         supp_var::MAX_BATCH_SIZE,
                         std::inserter(result, result.end()));

            if (auto stream_it = result.find(supp_var::STREAM_NAME); stream_it != result.end()) {
                for (const auto entry : stream_it->second) {
                    try {
                        SensorData data = iot::parser::RedisStreamParser::parse_sensor_data(entry.first, entry.second);

                        // std::cout << line_counter++ << " : " << data.device_id() << '\n';

                        aggregator->add_value(
                            data.device_id(),
                            data.temperature(),
                            data.timestamp().seconds());

                        alert_manager->check_thresholds(data);

                        last_id = entry.first;
                    } catch (const iot::parser::StreamParserException &e) {
                        std::cerr << "Pasre error: " << e.what() << '\n';
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Redis error: " << e.what() << '\n';
            std::this_thread::sleep_for(1s);
        }
    }
}

void setup_metrics(){
    auto registry = std::make_shared<prometheus::Registry>();
    auto& message_counter = prometheus::BuildCounter()
        .Name("message_processed_total")
        .Help("Total processed messages")
        .Register(*registry)
        .Add({});
    
        auto exporter = std::make_unique<prometheus::Exposer>(supp_var::PROMETHEUS_ADDRESS);
        exporter->RegisterCollectable(registry);
}

int main(){
    std::cout << "anal service/\n";
    std::signal(SIGINT, [](int){shutdown_requested.store(true);});
	std::signal(SIGTERM, [](int){shutdown_requested.store(true);});

    setup_metrics();

    auto aggregator = std::make_shared<MetricAggregator>();
    auto alert_manager = std::make_shared<AlertManager>();

    std::jthread redis_thread(redis_consumer,aggregator,alert_manager);

    AnalyticsServiceImpl service(aggregator,alert_manager);
    ServerBuilder builder;
    builder.AddListeningPort(supp_var::ANALYS_SERV_ADDRESS,grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Analytics service listened on port " << supp_var::ANALYS_SERV_ADDRESS << '\n';

    server->Wait();
    return 0;
}
