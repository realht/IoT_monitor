#pragma once

#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <memory>
#include <string>
#include "prometheus_metrics_publisher.h"

namespace iot::metrics {

class PrometheusMetrics{
public:
    PrometheusMetrics(const std::string address)
      : exposer_(address),
        registry_(std::make_shared<prometheus::Registry>()),
        publisher_(registry_){
            exposer_.RegisterCollectable(registry_);
            setup_metrics();
    }

    prometheus::Counter& message_processed() {return *message_processed_;}
    prometheus::Counter& parse_error() {return *parse_error_;}
    prometheus::Counter& alerts_triggered_temperature() {return *alerts_triggered_temperature_;}
    prometheus::Counter& alerts_triggered_humidity() {return *alerts_triggered_humidity_;}
    prometheus::Gauge& alert_rate() {return *alert_rate_;}
    prometheus::Histogram& processed_time() {return *processed_time_;}
    prometheus::Histogram& end_to_end_lanency() {return *end_to_end_latency_;}
    prometheus::Counter& message_acked() {return *message_acked_;}
    prometheus::Counter& ack_error() {return *ack_error_;}
    prometheus::Counter& claim_errors() {return *claim_error_;}
    prometheus::Family<prometheus::Counter>& message_processed_per_device() {return *message_processed_per_device_;}

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
        size_t window_size_humid){
            publisher_.publishMetrics(device_id,
            avg_temp,min_temp,max_temp,stddev_temp,
            avg_humid,min_humid,max_humid,stddev_humid,
            window_size_temp,window_size_humid);
        }

private:
    prometheus::Exposer exposer_;
    std::shared_ptr<prometheus::Registry> registry_;
    MetricPublisher publisher_;

    //указатели на метрики
    prometheus::Counter* message_processed_;
    prometheus::Counter* parse_error_;
    prometheus::Counter* alerts_triggered_temperature_;
    prometheus::Counter* alerts_triggered_humidity_;
    prometheus::Histogram* processed_time_;
    prometheus::Histogram* end_to_end_latency_;
    prometheus::Gauge* alert_rate_;
    prometheus::Counter* message_acked_;
    prometheus::Counter* ack_error_;
    prometheus::Counter* claim_error_;
    prometheus::Family<prometheus::Counter>* message_processed_per_device_;

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

        alerts_triggered_temperature_ = &prometheus::BuildCounter()
            .Name("alerts_triggered_temperature_total")
            .Help("Total triggered temperature errors")
            .Register(*registry_)
            .Add({});

        alerts_triggered_humidity_ = &prometheus::BuildCounter()
            .Name("alerts_triggered_humidity_total")
            .Help("Total triggered humidity errors")
            .Register(*registry_)
            .Add({});

        alert_rate_ = &prometheus::BuildGauge()
            .Name("alert_rate")
            .Help("Rate total message to alert")
            .Register(*registry_)
            .Add({});

        prometheus::Histogram::BucketBoundaries bb({0.001, 0.01, 0.05, 0.1, 0.5, 1.0});
        processed_time_ = &prometheus::BuildHistogram()
            .Name("processed_time_seconds")
            .Help("Message processing time distribution")
            .Register(*registry_)
            .Add({}, bb);

        end_to_end_latency_ = &prometheus::BuildHistogram()
            .Name("end_to_end_latency_seconds")
            .Help("End to end time latency")
            .Register(*registry_)
            .Add({}, bb);
        
        message_acked_ = &prometheus::BuildCounter()
            .Name("redis_messages_acked_total")
            .Help("Total successfuly acknowledged messages")
            .Register(*registry_)
            .Add({});

        ack_error_ = &prometheus::BuildCounter()
            .Name("redis_ack_error_total")
            .Help("Total ACK operation failures")
            .Register(*registry_)
            .Add({});

        claim_error_ = &prometheus::BuildCounter()
            .Name("redis_claim_error_total")
            .Help("Total message claim failures")
            .Register(*registry_)
            .Add({});

        message_processed_per_device_ = &prometheus::BuildCounter()
            .Name("message_processed_per_device")
            .Help("Total message processed per device")
            .Register(*registry_);
            
    }
};

} //namespace iot::metrics

