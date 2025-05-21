#pragma once

#include <string>
#include <cstdlib>
#include <chrono>
#include <spdlog/spdlog.h>

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

struct LogSetting{
    static std::string log_dir(){
        const char* env = std::getenv("LOG_DIR");
        return env ? env : "./logs";
    }

    static spdlog::level::level_enum log_level(){
        const char* env = std::getenv("LOG_LEVEL");
		if (!env) return spdlog::level::info;

        std::string level(env);
        std::transform(level.begin(), level.end(), level.begin(), ::tolower);
        
        if (level == "trace") return spdlog::level::trace;
        if (level == "debug") return spdlog::level::debug;
        if (level == "warn") return spdlog::level::warn;
        if (level == "error") return spdlog::level::err;
        if (level == "critical") return spdlog::level::critical;
        return spdlog::level::info;
    }

    static size_t rotate_size(){
        const char* env = std::getenv("LOG_ROTATE_SIZE");
        return env ? std::stoul(env) : 5242880;
    }

    static int rotate_files(){
        const char* env = std::getenv("LOG_ROTATE_FILES");
        return env ? std::stoi(env) : 3;
    }

    static size_t async_queue_size(){
        const char* env = std::getenv("LOG_ASYNC_QUEUE_SIZE");
        return env ? std::stoul(env) : 8196;
    }

    static int async_threads(){
        const char* env = std::getenv("LOG_ASYNC_THREADS");
        return env ? std::stoi(env) : 4;
    }

    static spdlog::level::level_enum flush_level(){
        const char* env = std::getenv("LOG_FLUSH_LEVEL");
		if (!env) return spdlog::level::warn;
        
        std::string level(env);
        std::transform(level.begin(), level.end(), level.begin(), ::tolower);
        
        if (level == "trace") return spdlog::level::trace;
        if (level == "debug") return spdlog::level::debug;
        if (level == "info") return spdlog::level::info;
        if (level == "warn") return spdlog::level::warn;
        if (level == "error") return spdlog::level::err;
        if (level == "critical") return spdlog::level::critical;
        return spdlog::level::warn;
    }
};

}