#include <prometheus/gauge.h>
#include <prometheus/registry.h>


namespace iot::metrics {

// класс для публикации метрик с лейблом device_id
class MetricPublisher{
public:
    explicit MetricPublisher(std::shared_ptr<prometheus::Registry> registry);
    
    void publishMetrics(
        const std::string& device_id,
        double avg_temp,
        double min_temp,
        double max_temp,
        double stddev_temp,
        double avg_humid,
        double min_humid,
        double max_humid,
        double stddev_humid,
        size_t window_size_temp,
        size_t window_size_humid
    );
    
private: 
    //семейство метрик хранящих все Gauges
    prometheus::Family<prometheus::Gauge>& temp_window_family_;
    prometheus::Family<prometheus::Gauge>& humid_window_family_;
    prometheus::Family<prometheus::Gauge>& temp_avg_family_;
    prometheus::Family<prometheus::Gauge>& temp_min_family_;
    prometheus::Family<prometheus::Gauge>& temp_max_family_;
    prometheus::Family<prometheus::Gauge>& temp_stddev_family_;
    prometheus::Family<prometheus::Gauge>& humid_avg_family_;
    prometheus::Family<prometheus::Gauge>& humid_min_family_;
    prometheus::Family<prometheus::Gauge>& humid_max_family_;
    prometheus::Family<prometheus::Gauge>& humid_stddev_family_;
    
    //хранение сзоданных gauge для каждого device_id
    std::unordered_map<std::string, prometheus::Gauge*> temp_window_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> humid_window_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> temp_avg_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> temp_min_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> temp_max_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> temp_stddev_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> humid_avg_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> humid_min_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> humid_max_metrics_;
    std::unordered_map<std::string, prometheus::Gauge*> humid_stddev_metrics_;
   
    void publishGauge(const std::string& deivce_id, double value,
        prometheus::Family<prometheus::Gauge>& family,
        std::unordered_map<std::string, prometheus::Gauge*>& metric_map);
};

}//namespace iot::metrics