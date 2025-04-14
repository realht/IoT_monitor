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
#include <chrono>
#include <csignal>
#include <optional>
#include "common.pb.h"
#include "analytics_service.grpc.pb.h"
#include <string_view>
#include <iterator>
#include "redis_stream_parser.h"
#include "prometheus_metrics.h"
#include "config.h"

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

class MetricAggregator {
public:
    struct Metrics{
        std::deque<std::pair<double,int64_t>> values;
        double sum = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();
    };

    void add_value(const std::string& device_id, double value, int64_t timespamp,
                iot::metrics::PrometheusMetrics& metrics);
    std::optional<Metrics> get_metrics(const std::string& device_id) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Metrics> metrics_;
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
    bool mathes_subscription(const Alert& alert, const AlertSubscription& sub);

    std::shared_ptr<MetricAggregator> aggregator_;
    std::shared_ptr<AlertManager> alert_manager_;
};

class AnalyticsServer{
public:
    AnalyticsServer() 
      : metrics_(iot::config::NetworkingSettings::prometheus_address()),
        aggregator_(std::make_shared<MetricAggregator>()),
        alert_manager_(std::make_shared<AlertManager>()) {}

    void Run();

private:
    iot::metrics::PrometheusMetrics metrics_;
    std::unique_ptr<Server> server_;
    std::shared_ptr<MetricAggregator> aggregator_;
    std::shared_ptr<AlertManager> alert_manager_;
    std::jthread redis_thread_;

    void setup_signal_handlers();
    void redis_consumer(std::shared_ptr<MetricAggregator> aggregator,
        std::shared_ptr<AlertManager> alert_manager,
        iot::metrics::PrometheusMetrics& metrics,
        std::stop_token st);
    void start_redis_consumer();
    void start_grpc_server();
    void wait_for_shutdown();
    void cleanup();
};