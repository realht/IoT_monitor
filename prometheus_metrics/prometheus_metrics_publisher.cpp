#include "prometheus_metrics_publisher.h"

namespace iot::metrics {

MetricPublisher::MetricPublisher(std::shared_ptr<prometheus::Registry> registry)
  : temp_window_family_(prometheus::BuildGauge()
                .Name("temp_aggregation_window")
                .Help("Current temperature aggregation window size per device")
                .Register(*registry)),
    humid_window_family_(prometheus::BuildGauge()
                .Name("humid_aggregation_window")
                .Help("Current humidity aggregation window size per device")
                .Register(*registry)),
    temp_avg_family_(prometheus::BuildGauge()
                .Name("temperature_avg")
                .Help("Average temterature per device")
                .Register(*registry)),
    temp_min_family_(prometheus::BuildGauge()
                .Name("temperature_min")
                .Help("Minimum temterature per device")
                .Register(*registry)),
    temp_max_family_(prometheus::BuildGauge()
                .Name("temperature_max")
                .Help("Maximum temterature per device")
                .Register(*registry)),
    temp_stddev_family_(prometheus::BuildGauge()
                .Name("temperature_stddev")
                .Help("Standard deviation of temterature per device")
                .Register(*registry)),
    humid_avg_family_(prometheus::BuildGauge()
                .Name("humidity_avg")
                .Help("Average hudimity per device")
                .Register(*registry)),
    humid_min_family_(prometheus::BuildGauge()
                .Name("humidity_min")
                .Help("Minimum hudimity per device")
                .Register(*registry)),
    humid_max_family_(prometheus::BuildGauge()
                .Name("humidity_max")
                .Help("Maximun hudimity per device")
                .Register(*registry)),
    humid_stddev_family_(prometheus::BuildGauge()
                .Name("humidity_stddev")
                .Help("Standard deviation of hudimity  per device")
                .Register(*registry)) {}

void MetricPublisher::publishGauge(const std::string& deivce_id, double value,
    prometheus::Family<prometheus::Gauge>& family,
    std::unordered_map<std::string, prometheus::Gauge*>& metric_map){
    auto it = metric_map.find(deivce_id);
    if(it == metric_map.end()){
        auto& gauge = family.Add({{"device_id", deivce_id}});
        auto inserted = metric_map.emplace(deivce_id,&gauge);
        it = inserted.first;
    }
    it->second->Set(value);
}

void MetricPublisher::publishMetrics(
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
    size_t window_size_humid){
    //window size
    publishGauge(device_id,static_cast<double>(window_size_temp),temp_window_family_, temp_window_metrics_);
    publishGauge(device_id,static_cast<double>(window_size_humid), humid_window_family_, humid_window_metrics_);


    //Temperature stats
    publishGauge(device_id, avg_temp,    temp_avg_family_,   temp_avg_metrics_);
    publishGauge(device_id, min_temp,    temp_min_family_,   temp_min_metrics_);
    publishGauge(device_id, max_temp,    temp_max_family_,   temp_max_metrics_);
    publishGauge(device_id, stddev_temp, temp_stddev_family_,temp_stddev_metrics_);

    //Humidity stats
    publishGauge(device_id, avg_humid,    humid_avg_family_,    humid_avg_metrics_);
    publishGauge(device_id, min_humid,    humid_min_family_,    humid_min_metrics_);
    publishGauge(device_id, max_humid,    humid_max_family_,    humid_max_metrics_);
    publishGauge(device_id, stddev_humid, humid_stddev_family_, humid_stddev_metrics_);

}

}