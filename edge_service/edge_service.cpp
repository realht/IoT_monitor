#include <iostream>
#include <grpcpp/grpcpp.h>
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


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;
using iot::v1::SensorData;
using iot::v1::Ack;
using iot::v1::EdgeService;

//глобальный флаг для graceful shutdown
std::atomic<bool> shutdown_requested{false};

//обработчик сигналов SIGINT и SIGTERM
void signal_handler(int signum){
	std::cout <<"\nSignal (" << signum<< ") recived. Initiating graceful shutdown...\n";
	shutdown_requested.store(true);
}

//потокобезопасная очередь для SensorData
class TreadSafeQueue{
public:
	void push(const SensorData& data){
		std::lock_guard<std::mutex> lock(mutex_);
		sd_queue_.push(data);
		cond_var_.notify_one();
	}

	//пытается извлечь данные. если очередь пуста блокируется до появления данных
	SensorData pop(){
		std::unique_lock<std::mutex> lock(mutex_);
		cond_var_.wait(lock, [this]() {return !sd_queue_.empty() || shutdown_;} );
		if(sd_queue_.empty()){
			//если очередь пуста возвращает пустой объект
			return SensorData();
		}
		SensorData data = sd_queue_.front();
		sd_queue_.pop();
		return data;
	}

	//сигнализирует о завершении работы очереди
	void shutdown(){
		std::lock_guard<std::mutex> lock(mutex_);
		shutdown_ = true;
		cond_var_.notify_all();
	}

	bool is_shutdown(){
		return shutdown_;
	}

private:
	std::queue<SensorData> sd_queue_;
	mutable std::mutex mutex_;
	std::condition_variable cond_var_;
	bool shutdown_ = false;
};


class EdgeServiceImpl final : public EdgeService::Service{
public:

	EdgeServiceImpl()
	: redis_(std::make_unique<sw::redis::Redis>("tcp://localhost:6379"))
		{
		// Проверка подключения к Redis
        try {
            redis_->ping();
            std::cout << "Connected to Redis successfully\n";
        } catch (const std::exception& e) {
            std::cerr << "Redis connection error: " << e.what() << "\n";
            exit(1);
        }
		for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
			workers_.emplace_back([this](std::stop_token st){
				//каждый поток непрерывно обрабатывает очередь
				while(!st.stop_requested() && !ts_queue_.is_shutdown()){
					SensorData data = ts_queue_.pop();
					if(data.device_id().empty()){
						continue;
					}
					std::vector<std::pair<std::string, std::string_view>> entires{
						{"device_id", data.device_id()},
						{"temperature", std::to_string(data.temperature())},
						{"humidity", std::to_string(data.humidity())},
						{"timestamp", std::to_string(data.timestamp().seconds())}};
					try{
						redis_->xadd(
							"telemetry_stream",
							"*",
							entires.begin(), entires.end());
						redis_->expire("telemetry_stream",3600*24);
					}catch(const std::exception& e){
						std::cerr << "Error writing to Redis: " << e.what() << '\n';
					}	
				}
			});
		}
	}

	~EdgeServiceImpl(){
		//сигнал рабочим потокам завершить работу
		ts_queue_.shutdown();
	}

	Status StreamTelemetry(ServerContext* context, 
		ServerReader<SensorData>* reader, Ack* ack) override{
			SensorData data;
			while(reader->Read(&data)){
				//write to console for test only
				std::cout << "Recivind data from device " << data.device_id() <<
				": Temperature=" << data.temperature() << ", Humidity=" <<
				data.humidity() << '\n';
				ts_queue_.push(data);
			}
			ack->set_success(true);
			ack->set_message("Data processing initiaded async");
			return Status::OK;
		}

private:
	std::unique_ptr<sw::redis::Redis> redis_;
	TreadSafeQueue ts_queue_; 
	std::vector<std::jthread> workers_;
};

void RunServer(){
	std::string server_address("0.0.0.0:50051");

	EdgeServiceImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address,grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Edge server listining on " << server_address << '\n';

	//поток для grace shutdown сервера
	std::jthread shutdown_thread([&](std::stop_token st){
		while(!st.stop_requested() && !shutdown_requested.load()){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if(shutdown_requested.load()){
			std::cout << "Shutdown signal detected. Shutting down EdgeServer gracefully...\n";
			server->Shutdown();
		}
	});

	//ожидаем завершение работы сервера
	server->Wait();
}

int main(){
	//регистрируем обработчики сигналов для SIGINT и SIGTERM
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);

	std::cout << "Greating to async grpc req server\n";
	RunServer();
	std::cout << "Server shutdown gracefully!\n";
	return 0;	
	}
