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
    prometheus::Gauge& aggregation_window() {return *aggregation_window_;}


private:
    prometheus::Exposer exposer_;
    std::shared_ptr<prometheus::Registry> registry_;

    //указатели на метрики
    prometheus::Counter* message_processed_;
    prometheus::Counter* parse_error_;
    prometheus::Counter* alerts_triggered_;
    prometheus::Histogram* processed_time_;
    prometheus::Gauge* aggregation_window_;

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
        
        aggregation_window_ = &prometheus::BuildGauge()
            .Name("aggregation_windows_size")
            .Help("Current aggregation window size")
            .Register(*registry_)
            .Add({});
    }
};

} //namespace iot::metrics

