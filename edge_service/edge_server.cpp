#include "edge_server.h"

std::atomic<bool> shutdown_requested{false};

template<typename T>
bool ThreadSafeQueue<T>::pop(T& value){
		std::unique_lock<std::mutex> lock(mutex_);
		cond_var_.wait(lock, [this]() {return !queue_.empty() || shutdown_;} );
		
		//если очередь пуста возвращает пустой объект
		if(queue_.empty()) return false;

		value = std::move(queue_.front());
		queue_.pop();
		return true;
	}

template <typename T>
bool ThreadSafeQueue<T>::pop_bulk(std::vector<T>& values, size_t max_count){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this]() {return !queue_.empty() || shutdown_;});

    if(queue_.empty()) return false;
 
    auto elements_to_take = std::min(max_count,queue_.size());
    auto end_it = std::next(queue_.begin(),elements_to_take);

    values.reserve(values.size() + elements_to_take);
    values.insert(values.end(),
                std::make_move_iterator(queue_.begin()),
                std::make_move_iterator(queue_.begin() + elements_to_take));
    queue_.erase(queue_.begin(),end_it);
    return true;
}

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
    size_t num_consumers = iot::config::StreamSettings::num_queue_shards;

    for(size_t i = 0;i < num_consumers;++i){
        redis_consumers_.emplace_back([this](std::stop_token st){
            auto redis = redis_pool_.get();
            auto batch_size = iot::config::StreamSettings::max_batch_size;
            auto batch_timeout = iot::config::StreamSettings::batch_timeout;
            std::vector<SensorData> batch;
            batch.reserve(batch_size);
            auto last_flush = std::chrono::steady_clock::now();

            while (!st.stop_requested()) {
                try {
                    //SensorData data;
                    //извлекаем пачку данных
                    std::vector<SensorData> temp_batch;
                    if(queue_.pop_bulk(temp_batch,batch_size)){
                        batch.insert(batch.end(),
                        std::make_move_iterator(temp_batch.begin()),
                        std::make_move_iterator(temp_batch.end()));
                    }

                    //проверяем условия отправки
                    auto now = std::chrono::steady_clock::now();
                    if ( batch.size() >= batch_size ||
                        (batch.size() > 0 && (now - last_flush) > batch_timeout) ){
                        write_batch_to_redis(batch,redis);
                        batch.clear();
                        last_flush = now;
                    }
                    std::this_thread::sleep_for(1ms);
                } catch (const std::exception& e) {
                    std::cerr << "Error writing to Redis: " << e.what() << '\n';
                    logger_->critical("Error writing to Redis: {}", e.what());
                    std::this_thread::sleep_for(100ms);
                }
            }

            if(!batch.empty()){
                try{
                    write_batch_to_redis(batch,redis);
                }catch(const std::exception& e){
                    std::cerr << "Error writing to Redis: " << e.what() << '\n';
                    logger_->critical("Error writing to Redis: {}", e.what());
                    std::this_thread::sleep_for(100ms);
                }
            }
        });
    };

}

void EdgeServer::write_batch_to_redis(const std::vector<SensorData>& batch,
                                    std::shared_ptr<sw::redis::Redis>& redis){
    try{
        //используем pipepline для массовой записи
        auto pipe = redis->pipeline(true);
        for (const auto& data : batch){
            std::vector<std::pair<std::string, std::string>> entries{
                {"device_id", data.device_id()},
                {"temperature", std::to_string(data.temperature())},
                {"humidity", std::to_string(data.humidity())},
                {"timestamp", std::to_string(data.timestamp().seconds())}};
            pipe.xadd(iot::config::StreamSettings::stream_name,
                        "*", entries.begin(), entries.end());
        }
        pipe.exec();
    }catch(const sw::redis::Error& e){
        logger_->error("Redis pipeline error: {}",e.what());
        throw;
    }
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
    logger_->info("Edge server initialized on port {}", server_address);
    
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

    for(auto& redis_thread : redis_consumers_){
        if (redis_thread.joinable()) {
            redis_thread.request_stop();
            redis_thread.join();
        }
    }
    
    if (grpc_thread_.joinable()) {
        grpc_thread_.request_stop();
        grpc_thread_.join();
    }
}