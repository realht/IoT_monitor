#pragma once

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <memory>
#include <string>

namespace iot::metrics {

class PrometheusMetrics{
public:
    PrometheusMetrics(const std::string address)
      : exposer_(address),
        registry_(std::make_shared<prometheus::Registry>()){
            exposer_.RegisterCollectable(registry_);
            setup_metrics();
    }

    prometheus::Counter& message_processed() {return *message_processed_;}
    prometheus::Counter& parse_error() {return *parse_error_;}
    prometheus::Counter& alerts_triggered() {return *alerts_triggered_;}
    prometheus::Histogram& processed_time() {return *processed_time_;}
    prometheus::Gauge& temp_aggregation_window() {return *temp_aggregation_window_;}
    prometheus::Gauge& humid_aggregation_window() {return *humid_aggregation_window_;}
    prometheus::Gauge& temperature_avg() {return *temperature_avg_;}
    prometheus::Gauge& temperature_min() {return *temperature_min_;}
    prometheus::Gauge& temperature_max() {return *temperature_max_;}
    prometheus::Gauge& humidity_avg() {return *humidity_avg_;}
    prometheus::Gauge& humidity_min() {return *humidity_min_;}
    prometheus::Gauge& humidity_max() {return *humidity_max_;}

private:
    prometheus::Exposer exposer_;
    std::shared_ptr<prometheus::Registry> registry_;

    //указатели на метрики
    prometheus::Counter* message_processed_;
    prometheus::Counter* parse_error_;
    prometheus::Counter* alerts_triggered_;
    prometheus::Histogram* processed_time_;
    prometheus::Gauge* temp_aggregation_window_;
    prometheus::Gauge* humid_aggregation_window_;
    prometheus::Gauge* temperature_avg_;
    prometheus::Gauge* temperature_min_;
    prometheus::Gauge* temperature_max_;
    prometheus::Gauge* humidity_avg_;
    prometheus::Gauge* humidity_min_;
    prometheus::Gauge* humidity_max_;


    void setup_metrics(){
        message_processed_ = &prometheus::BuildCounter()
            .Name("message_processed_total")
            .Help("Total processed messages")
            .Register(*registry_)
            .Add({});

        parse_error_ = &prometheus::BuildCounter()
            .Name("parse_errors_total")
            .Help("Total data perse errors")
            .Register(*registry_)
            .Add({});
        
        alerts_triggered_ = &prometheus::BuildCounter()
            .Name("alerts_triggered_total")
            .Help("Total triggered errors")
            .Register(*registry_)
            .Add({});

        prometheus::Histogram::BucketBoundaries bb({0.001, 0.01, 0.05, 0.1, 0.5, 1.0});
        processed_time_ = &prometheus::BuildHistogram()
            .Name("processed_time_seconds")
            .Help("Message processing time distribution")
            .Register(*registry_)
            .Add({}, bb);
        
        temp_aggregation_window_ = &prometheus::BuildGauge()
            .Name("temp_aggregation_window")
            .Help("Current temperature aggregation window size")
            .Register(*registry_)
            .Add({});

        humid_aggregation_window_ = &prometheus::BuildGauge()
            .Name("humid_aggregation_window")
            .Help("Current humidity aggregation window size")
            .Register(*registry_)
            .Add({});

        temperature_avg_ = &prometheus::BuildGauge()
            .Name("temperature_avg")
            .Help("Average temterature")
            .Register(*registry_)
            .Add({});

        temperature_min_ = &prometheus::BuildGauge()
            .Name("temperature_min")
            .Help("Min temterature")
            .Register(*registry_)
            .Add({});

        temperature_max_ = &prometheus::BuildGauge()
            .Name("temperature_max")
            .Help("Max temterature")
            .Register(*registry_)
            .Add({});

        humidity_avg_ = &prometheus::BuildGauge()
            .Name("humidity_avg")
            .Help("Average hudimity")
            .Register(*registry_)
            .Add({});

        humidity_min_ = &prometheus::BuildGauge()
            .Name("humidity_min")
            .Help("Min hudimity")
            .Register(*registry_)
            .Add({});

        humidity_max_ = &prometheus::BuildGauge()
            .Name("humidity_max")
            .Help("Max hudimity")
            .Register(*registry_)
            .Add({});
    }
};

} //namespace iot::metrics

