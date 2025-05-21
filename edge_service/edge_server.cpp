#include "edge_server.h"

std::atomic<bool> shutdown_requested{false};

void EdgeServiceImpl::CallData::Proceed(bool ok) {
    if (status_ == CREATE) {
        status_ = PROCESSING;
        service_->RequestStreamTelemetry(&ctx_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESSING) {
        if (shutdown_requested.load()) {  // Проверка завершения
            delete this;
            return;
        }
        // Новый запрос принят, создаем новый CallData для следующего запроса
        new CallData(service_, cq_);
        status_ = READING;
        responder_.Read(&request_, this);
        ///std::cout << "data processed\n";				
    } else if (status_ == READING) {
        if (!ok) {
            // Клиент завершил передачу, завершаем поток
            status_ = FINISHED;
            ack_.set_success(true);
            ack_.set_message("Stream processed asyncly");
            responder_.Finish(ack_, Status::OK, this);
            return;
        } 
        // Данные прочитаны, отправляем в очередь
        service_->queue_.push(request_);
        // Продолжаем читать следующее сообщение
        responder_.Read(&request_, this);
    } else {
            delete this;
    }
}

void EdgeServer::Run() {
    setup_signal_handlers();
    start_redis_consumer();
    start_grpc_server();
    wait_for_shutdown();
    cleanup();
}

void EdgeServer::setup_signal_handlers() {
    std::signal(SIGINT, [](int) { 
        std::cout << "SIGINT received.\n";
        shutdown_requested.store(true); });
    std::signal(SIGTERM, [](int) { 
        std::cout << "SIGTERM received.\n";
        shutdown_requested.store(true); });
}

void EdgeServer::start_redis_consumer() {
    redis_thread_ = std::jthread([this](std::stop_token st) {
        auto redis = redis_pool_.get();
        while (!st.stop_requested()) {
            try {
                SensorData data;
                if (queue_.pop(data)) {
                    std::vector<std::pair<std::string, std::string>> entries{
                        {"device_id", data.device_id()},
                        {"temperature", std::to_string(data.temperature())},
                        {"humidity", std::to_string(data.humidity())},
                        {"timestamp", std::to_string(data.timestamp().seconds())}};
                    redis->xadd(iot::config::StreamSettings::stream_name, 
                        "*", entries.begin(), entries.end());
                    //message_processed_counter_.Add({}).Increment();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error writing to Redis: " << e.what() << '\n';
            }
        }
    });
}

void EdgeServer::start_grpc_server() {
    ServerBuilder builder;
    std::string server_address = iot::config::NetworkingSettings::edge_service_address();
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Edge server listening on port " << server_address << '\n';
    logger_->info("Edge server initialized");
    
    service_->StartPrecessing(cq_.get());
    
    grpc_thread_ = std::jthread([this](std::stop_token st) {
        void* tag;
        bool ok;
        constexpr auto poll_interval = iot::config::StreamSettings::write_timeout;
    
        while (!st.stop_requested()) {
            auto deadline = std::chrono::system_clock::now() + poll_interval;
            auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::system_clock::now());
    
            CompletionQueue::NextStatus status = cq_->AsyncNext(
                &tag, &ok, gpr_time_add(
                    gpr_now(GPR_CLOCK_MONOTONIC),
                    gpr_time_from_millis(timeout.count(), GPR_TIMESPAN)));
    
            if (status == grpc::CompletionQueue::GOT_EVENT) {
                if (tag) static_cast<EdgeServiceImpl::CallData*>(tag)->Proceed(ok);
            } else if (status == grpc::CompletionQueue::SHUTDOWN) {
                break;
            }
        }
        cq_->Shutdown();
    });
}

void EdgeServer::wait_for_shutdown() {
    while (!shutdown_requested.load()) {
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "Shutdown signal detected. Shutting down EdgeServer gracefully...\n";
    server_->Shutdown();
    server_->Wait();
}

void EdgeServer::cleanup() {
    queue_.shutdown();

    if (redis_thread_.joinable()) {
        redis_thread_.request_stop();
        redis_thread_.join();
    }
    if (grpc_thread_.joinable()) {
        grpc_thread_.request_stop();
        grpc_thread_.join();
    }
}