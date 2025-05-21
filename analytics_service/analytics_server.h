#include <iostream>
#include <sw/redis++/redis++.h>
#include <grpcpp/grpcpp.h>
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
#include <csignal>
#include <optional>
#include "common.pb.h"
#include "analytics_service.grpc.pb.h"
#include <string_view>
#include <iterator>
#include "redis_stream_parser.h"
#include "prometheus_metrics.h"
#include "config.h"
#include <google/protobuf/util/time_util.h>
#include "logger_factory.h"

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

extern size_t line_counter;
extern std::atomic<bool> shutdown_requested;
extern const double EPS;

class MetricAggregator {
public:
    struct MetricData{
        std::deque<std::pair<double,int64_t>> values;
        double sum = 0.0;
        double sum_squares = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();

        double compute_stddev();
    };
    struct Metrics{
        MetricData temperature;
        MetricData humidity;
    };

    void add_values(const std::vector<SensorData>& batch, iot::metrics::PrometheusMetrics& metrics);
    void add_value(const std::string& device_id, double temp_value, double humidity_value, 
                    int64_t timestamp, iot::metrics::PrometheusMetrics& metrics);
    std::optional<Metrics> get_metrics(const std::string& device_id) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Metrics> metrics_;

    void update_metric(MetricData& metric, double value, int64_t timestamp);
    void update_metrics_for_device(const std::string& device_id, double temp_value, 
        double humidity_value, int64_t timestamp, iot::metrics::PrometheusMetrics& metrics);
};

class AlertManager {
public:
    void check_thresholds(const SensorData& data, 
                        iot::metrics::PrometheusMetrics& metrics);
    std::vector<Alert> get_alerts();

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
                    ServerWriter<StatResponse>* writer) override;
    Status SubscribeToAlerts(ServerContext* context, const AlertSubscription* sub,
                    ServerWriter<Alert>* writer) override;

private:
    bool matches_subscription(const Alert& alert, const AlertSubscription& sub);

    std::shared_ptr<MetricAggregator> aggregator_;
    std::shared_ptr<AlertManager> alert_manager_;
};

class AnalyticsServer{
public:
    AnalyticsServer() 
      : metrics_(iot::config::NetworkingSettings::prometheus_address()),
        aggregator_(std::make_shared<MetricAggregator>()),
        alert_manager_(std::make_shared<AlertManager>()),
        logger_(iot::logging::LoggerFactory::create_service_logger("analytics_server")) {}

    void Run();

private:
    iot::metrics::PrometheusMetrics metrics_;
    std::unique_ptr<Server> server_;
    std::unique_ptr<AnalyticsServiceImpl> service_;
    std::shared_ptr<MetricAggregator> aggregator_;
    std::shared_ptr<AlertManager> alert_manager_;
    std::jthread redis_thread_;
    std::shared_ptr<spdlog::logger> logger_;

    void setup_signal_handlers();
    void redis_consumer(std::shared_ptr<MetricAggregator> aggregator,
                        std::shared_ptr<AlertManager> alert_manager,
                        iot::metrics::PrometheusMetrics& metrics,
                        std::stop_token st);
    void start_redis_consumer();
    void start_grpc_server();
    void wait_for_shutdown();
    void cleanup();

    void process_entry(
        const std::pair<std::string, iot::parser::RedisStreamParser::FieldMap>& entry,
        MetricAggregator& aggregator,
        AlertManager& alert_manager,
        iot::metrics::PrometheusMetrics& metrics);
        
    void ack_messages(
        sw::redis::Redis& redis,
        const std::string& stream,
        const std::string& group,
        std::vector<std::string>& ids,
        iot::metrics::PrometheusMetrics& metrics);
    
    void update_metrics(
        iot::metrics::PrometheusMetrics& metrics,
        std::chrono::steady_clock::time_point start,
        size_t precessed_count);

};