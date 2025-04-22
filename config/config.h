#pragma once

#include <string>
#include <cstdlib>
#include <chrono>

namespace iot::config {

struct NetworkingSettings {
    //Redis
    static std::string redis_host() {
        const char* env = std::getenv("REDIS_HOST");
        return env ? env : "tcp://localhost:6379";
    }

    //Edge Service
    static std::string edge_service_address(){
        const char* env = std::getenv("EDGE_SERVICE_ADDRESS");
        return env ? env : "0.0.0.0:50051";
    }

    //Analytics Service
    static std::string analytics_service_address(){
        const char* env = std::getenv("ANALYTICS_SERVICE_ADDRESS");
        return env ? env : "0.0.0.0:50052";
    }

    //Prometheus
    static std::string prometheus_address(){
        const char* env = std::getenv("PROMETHEUS_ADDRESS");
        return env ? env : "0.0.0.0:8081";
    }
};

struct StreamSettings {
    static constexpr size_t max_batch_size = 10;
    static constexpr auto write_timeout = std::chrono::milliseconds(10);
    static constexpr auto read_timeout = std::chrono::milliseconds(100);
    static constexpr const char* stream_name = "telemetry_stream";
    static constexpr const char* alerts_stream = "alerts_stream";
    static constexpr const char* consumer_group = "analytics_group";
    static constexpr const int pendint_timeout_ms = 5000;
    static constexpr const int max_retries = 3;

    static inline std::string consumer_name() {
        static std::string name = [](){
            char hostname[256] = {0};
            gethostname(hostname, sizeof(hostname));
            return std::string(hostname) + "_" + std::to_string(getpid());
        }();
        return name;
    }
};

struct Thresholds {
    static constexpr double temperature = 50.0;
    static constexpr double humidity = 60.0;
    static constexpr size_t aggregation_window_sec = 30;
};

}