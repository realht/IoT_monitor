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
bool ThreadSafeQueue<T>::pop_bulk(std::vector<T>& values, size_t max_count,
                                std::chrono::milliseconds timeout){
    std::unique_lock<std::mutex> lock(mutex_);
    auto predicate = [this] {return !queue_.empty() || shutdown_;};

    if(!cond_var_.wait_for(lock,timeout, predicate)){
        return false;
    }

    if(queue_.empty() || shutdown_)  return false;
 
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
        size_t shard = std::hash<std::string>{}(request_.device_id()) % service_->queues_.size();
        service_->queues_[shard]->push(request_);

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
    const size_t num_consumers = iot::config::StreamSettings::num_queue_shards;
    const auto batch_timeout = iot::config::StreamSettings::batch_timeout;
    const auto batch_size = iot::config::StreamSettings::max_batch_size;

    redis_consumers_.reserve(num_consumers);
    for(size_t i = 0; i < num_consumers; ++i){
        redis_consumers_.emplace_back([this, i, batch_timeout, batch_size](std::stop_token st){
            auto redis = redis_pool_.get();
            auto& queue = *queues_[i];
            std::vector<SensorData> batch;
            batch.reserve(batch_size);
            auto last_flush = std::chrono::steady_clock::now();

            while (!st.stop_requested()) {
                try {
                    //извлекаем пачку данных
                    std::vector<SensorData> temp_batch;
                    if(queue.pop_bulk(temp_batch,batch_size, batch_timeout)){
                        batch.insert(batch.end(),
                                    std::make_move_iterator(temp_batch.begin()),
                                    std::make_move_iterator(temp_batch.end()));
                    }

                    //проверяем условия отправки
                    auto now = std::chrono::steady_clock::now();
                    if ( batch.size() >= batch_size ||
                        (!batch.empty() && (now - last_flush) > batch_timeout) ){
                        write_batch_to_redis(batch,redis);
                        batch.clear();
                        last_flush = now;
                    }
                    std::this_thread::sleep_for(1ms);
                } catch (const std::exception& e) {
                    logger_->critical("Error writing to Redis: {}", e.what());
                    std::this_thread::sleep_for(100ms);
                }
            }

            if(!batch.empty()){
                try{
                    write_batch_to_redis(batch,redis);
                }catch(const std::exception& e){
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

    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
 
    const size_t num_threads = std::min(4u, std::thread::hardware_concurrency());
    auto cq = builder.AddCompletionQueue();
    cqs_.emplace_back(std::move(cq));

    server_ = builder.BuildAndStart();
    std::cout << "Edge server listening on port " << server_address << "for core: " << num_threads << '\n';
    logger_->info("Edge server initialized on port {}", server_address);


    for(size_t i = 0; i < num_threads; ++i){

        grpc_threads_.emplace_back([this, cq = cqs_.front().get()](std::stop_token st){

            //Запуск обработчиков
            for(size_t i =0;i<2;++i){
                service_->StartProcessing(cq);
            }

            void* tag;
            bool ok;
            while (!st.stop_requested()){
                const auto deadline = std::chrono::system_clock::now() + 
                                        iot::config::StreamSettings::write_timeout;
                auto status = cq->AsyncNext(&tag, &ok,deadline);

                switch(status){
                    case grpc::CompletionQueue::GOT_EVENT:
                        if(tag != nullptr){
                            auto* call_data = static_cast<EdgeServiceImpl::CallData*>(tag);
                            call_data->Proceed(ok);
                        }else{
                            logger_->error("Null tag received in GOT_EVENT");
                        }
                        break;
                    case grpc::CompletionQueue::SHUTDOWN:
                        logger_->info("CompletionQueue shutdown initiated");
                        return;
                    default:
                        logger_->warn("Unexpected CompletionQueue status {}", static_cast<int>(status));
                        break;
                }
            }
            logger_->debug("gRPC processing thread exiting");
        });
    }
}

void EdgeServer::wait_for_shutdown() {
    while (!shutdown_requested.load()) {
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "Shutdown signal detected. Shutting down EdgeServer gracefully...\n";
    logger_->info("Shutdown signal detected. Shutting down EdgeServer gracefully...");
    server_->Shutdown();
    server_->Wait();
}

size_t EdgeServer::get_shred_index(const std::string& key){
    return std::hash<std::string>{}(key) % queues_.size();
}

void EdgeServer::cleanup() {

    for(auto& queue : queues_){
        queue->shutdown();
    }
    queues_.clear();

    for(auto& redis_thread : redis_consumers_){
        if (redis_thread.joinable()) {
            redis_thread.request_stop();
            redis_thread.join();
        }
    }
    redis_consumers_.clear();
    
    for(auto& grpc_thread : grpc_threads_){
        if (grpc_thread.joinable()) {
            grpc_thread.request_stop();
            grpc_thread.join();
        }
    }
    grpc_threads_.clear();

    for(auto& cq : cqs_){
        cq->Shutdown();
    }
    cqs_.clear();
    
}