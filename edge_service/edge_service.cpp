#include <iostream>
#include <grpcpp/grpcpp.h>
#include "common.pb.h"
#include "edge_service.grpc.pb.h"
#include <sw/redis++/redis++.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <csignal>
#include <sys/resource.h>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerAsyncReader;
using grpc::CompletionQueue;
using grpc::ServerCompletionQueue;
using grpc::Status;
using iot::v1::SensorData;
using iot::v1::Ack;
using iot::v1::EdgeService;

namespace supp_var{
	const std::string REDIS_SERV_ADRESS = "tcp://localhost:6379";
	const std::string EGDE_SERV_ADDRESS = "0.0.0.0:50051";
	const std::string STREAM_NAME = "telemetry_stream";
    constexpr size_t ASYNC_INTERVAL_MS = 10;
    constexpr size_t MAX_CONCURRENT_STREAMS = 1000000;
}



//глобальный флаг для graceful shutdown
std::atomic<bool> shutdown_requested{false};

//потокобезопасная очередь для SensorData
template <typename T>
class ThreadSafeQueue{
public:
	void push(const T& value){
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(value);
		cond_var_.notify_one();
	}

	//пытается извлечь данные. если очередь пуста блокируется до появления данных
	bool pop(T& value){
		std::unique_lock<std::mutex> lock(mutex_);
		cond_var_.wait(lock, [this]() {return !queue_.empty() || shutdown_;} );
		
		//если очередь пуста возвращает пустой объект
		if(queue_.empty()) return false;

		value = std::move(queue_.front());
		queue_.pop();
		return true;
	}

	//сигнализирует о завершении работы очереди
	void shutdown(){
		std::lock_guard<std::mutex> lock(mutex_);
		shutdown_ = true;
		cond_var_.notify_all();
	}

private:
	std::queue<T> queue_;
	mutable std::mutex mutex_;
	std::condition_variable cond_var_;
	bool shutdown_ = false;
};


class EdgeServiceImpl final : public EdgeService::AsyncService{
public:
	explicit EdgeServiceImpl(size_t thread_pool_size)
	: redis_pool_(thread_pool_size){
		// запуск потоков
		for(size_t i = 0;i < thread_pool_size;i++){
			workers_.emplace_back([this](std::stop_token st) {
				SensorData data;
				while(!st.stop_requested()){
					if(queue_.pop(data)) processSensorData(data);
				}
			});
		}
	}

	~EdgeServiceImpl(){
		//сигнал рабочим потокам завершить работу
		queue_.shutdown();
	}

	void StartPrecessing(ServerCompletionQueue* cq){
		new CallData(this, cq);
	}

	
private:
	friend void RunServer();

	class CallData{
		public:
		CallData(EdgeServiceImpl* service, ServerCompletionQueue* cq)
			: service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
			{Proceed();}

		
		void Proceed(bool ok = true) {
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


		private:
			EdgeServiceImpl* service_;
			ServerCompletionQueue* cq_;
			ServerContext ctx_;
			ServerAsyncReader<Ack, SensorData> responder_;
			SensorData request_;
			Ack ack_;
			enum CallStatus{CREATE, PROCESSING, READING,FINISHED};
			CallStatus status_;
	};

	class RedisPool{
		public:
		RedisPool(size_t size){
			for(size_t i=0;i<size;i++){
				pool_.emplace_back(std::make_shared<sw::redis::Redis>(
					supp_var::REDIS_SERV_ADRESS));

			}
		}

		std::shared_ptr<sw::redis::Redis> get(){
			// static std::atomic<size_t> index{0};
			// return pool_[index++ % pool_.size()];
			thread_local static size_t index = 0;
			return pool_[index++ % pool_.size()];
		}

		private:
			std::vector<std::shared_ptr<sw::redis::Redis>> pool_;

	};

	void processSensorData(const SensorData& data){
		try{
			auto redis = redis_pool_.get();
						
			std::vector<std::pair<std::string, std::string>> entires{
				{"device_id", data.device_id()},
				{"temperature", std::to_string(data.temperature())},
				{"humidity", std::to_string(data.humidity())},
				{"timestamp", std::to_string(data.timestamp().seconds())}};
			redis->xadd(supp_var::STREAM_NAME, "*",
					entires.begin(),entires.end());
			//std::cout << "data: " << entires.front().second<< '\n';
		}catch(const std::exception& e){
			std::cerr << "Error writing to Redis: " << e.what() << '\n';
		}
	}

	ThreadSafeQueue<SensorData> queue_;
	RedisPool redis_pool_;
	std::vector<std::jthread> workers_;
};

void RunServer(){
	EdgeServiceImpl service(std::thread::hardware_concurrency());
	ServerBuilder builder;

	//настройка асинхронного сервера и добавляем компрессию
	builder.AddListeningPort(supp_var::EGDE_SERV_ADDRESS,grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);

	auto cq = builder.AddCompletionQueue();
	auto server = builder.BuildAndStart();
	service.StartPrecessing(cq.get());

	std::cout << "Edge server v3 listining on " << supp_var::EGDE_SERV_ADDRESS << '\n';

	//Поток для обработки GRPC
	std::jthread cq_thread([cq = cq.get()](std::stop_token st){
		void* tag;
		bool ok;
		constexpr auto poll_interval = std::chrono::milliseconds(supp_var::ASYNC_INTERVAL_MS);

		while(!st.stop_requested()){
			auto deadline = std::chrono::system_clock::now() + poll_interval;
			auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
				deadline - std::chrono::system_clock::now());

			CompletionQueue::NextStatus status = cq->AsyncNext(
				&tag, &ok, gpr_time_add(
					gpr_now(GPR_CLOCK_MONOTONIC),
					gpr_time_from_millis(timeout.count(),GPR_TIMESPAN))
			);

			if(status == grpc::CompletionQueue::GOT_EVENT){
				if(tag) static_cast<EdgeServiceImpl::CallData*>(tag)->Proceed(ok);
			}else if(status == grpc::CompletionQueue::SHUTDOWN){
				break;
			}
		}
	});


	//поток для grace shutdown сервера
	std::jthread shutdown_thread([&](std::stop_token st){
		while(!st.stop_requested()){
			if(shutdown_requested.load()){
				std::cout << "Shutdown signal detected. Shutting down EdgeServer gracefully...\n";
				server->Shutdown();
				cq->Shutdown();
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});

	//ожидаем завершение работы сервера
	server->Wait();

	//очищение очереди
	void* tag;
	bool ok;
	while (cq->Next(&tag, &ok)) {
		if (tag) {
		static_cast<EdgeServiceImpl::CallData*>(tag)->Proceed(ok);
		}
	}
}

int main(){
	//регистрируем обработчики сигналов для SIGINT и SIGTERM
	std::signal(SIGINT, [](int){shutdown_requested.store(true);});
	std::signal(SIGTERM, [](int){shutdown_requested.store(true);});

	std::cout << "Greating to async grpc req server\n";
	RunServer();
	std::cout << "Server shutdown gracefully!\n";
	return 0;	
	}
