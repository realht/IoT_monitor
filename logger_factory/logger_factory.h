#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>
#include <chrono>
#include "config.h"

namespace iot::logging{

class LoggerFactory{
public:
    LoggerFactory() = delete;

    static std::shared_ptr<spdlog::logger> create_service_logger(const std::string name){
        init_once();
        return create_async_logger(name,
                    iot::config::LogSetting::log_dir(),
                    iot::config::LogSetting::rotate_size(),
                    iot::config::LogSetting::rotate_files(),
                    iot::config::LogSetting::log_level()
        );
    }
    
private:
    static void init_once(){
        static std::once_flag init_flag;
        using namespace std::chrono_literals;
        std::call_once(init_flag,[](){
            spdlog::init_thread_pool(
                iot::config::LogSetting::async_queue_size(),
                iot::config::LogSetting::async_threads()
            );
            spdlog::flush_every(std::chrono::seconds(5s));
        });
    }

    static std::shared_ptr<spdlog::logger> create_async_logger(
        const std::string& name,
        const std::string& log_dir,
        size_t max_size,
        size_t max_files,
        spdlog::level::level_enum level){
            try{
                auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    log_dir + "/" + name + ".log",
                    max_size,
                    max_files
                );

                auto logger = std::make_shared<spdlog::async_logger>(
                    name,
                    sink,
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::overrun_oldest
                );

                logger->set_level(level);
                logger->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [%n] [tid:%t] %v");
                logger->flush_on(spdlog::level::warn);

                spdlog::register_logger(logger);
                return logger;        
            }catch(const spdlog::spdlog_ex& ex){
                throw std::runtime_error("Logger initialization error " + std::string(ex.what()));
            }
    }
};

}
