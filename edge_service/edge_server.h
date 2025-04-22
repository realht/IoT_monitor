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
#include "prometheus_metrics.h"
#include "config.h"

using namespace std::chrono_literals;
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

extern std::atomic<bool> shutdown_requested;

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
    explicit EdgeServiceImpl(ThreadSafeQueue<SensorData>& queue)
    : queue_(queue){}
    
    ~EdgeServiceImpl(){
        //сигнал рабочим потокам завершить работу
        queue_.shutdown();
    }
    
    void StartPrecessing(ServerCompletionQueue* cq){
        new CallData(this, cq);
    }   
        
private:
    friend class EdgeServer;
    
    class CallData{
    public:
        CallData(EdgeServiceImpl* service, ServerCompletionQueue* cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
            {Proceed();}
        
        void Proceed(bool ok = true);
    
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
    
    ThreadSafeQueue<SensorData>& queue_;
};


class EdgeServer {
public:
    EdgeServer()
    :   redis_pool_(std::thread::hardware_concurrency()),
        queue_(),
        service_(std::make_unique<EdgeServiceImpl>(queue_)) {}
        
    void Run();
        
private:
    // prometheus::Exposer metrics_exposer_{supp_var::EDGE_SERV_ADDRESS};
    // prometheus::Registry metrics_registry_;
    // prometheus::Family<prometheus::Counter>& message_processed_counter_ =
    //     prometheus::BuildCounter()
    //         .Name("messages_processed_total")
    //         .Help("Total number of processed messages")
    //         .Register(metrics_registry_);
        
    class RedisPool {
    public:
        RedisPool(size_t size) {
            for (size_t i = 0; i < size; ++i) {
                pool_.emplace_back(std::make_shared<sw::redis::Redis>
                    (iot::config::NetworkingSettings::redis_host()));
            }
        }
        
        std::shared_ptr<sw::redis::Redis> get() {
            thread_local static size_t index = 0;
            return pool_[index++ % pool_.size()];
        }
        
    private:
        std::vector<std::shared_ptr<sw::redis::Redis>> pool_;
    };
        
    RedisPool redis_pool_;
    ThreadSafeQueue<SensorData> queue_;
    //EdgeServiceImpl service_;
    std::unique_ptr<EdgeServiceImpl> service_;
    std::unique_ptr<Server> server_;
    std::unique_ptr<ServerCompletionQueue> cq_; 
    std::jthread redis_thread_;
    std::jthread grpc_thread_;
        
    void setup_signal_handlers();
    void start_redis_consumer();     
    void start_grpc_server();
    void wait_for_shutdown();
    void cleanup();
};