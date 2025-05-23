#include <iostream>
#include <grpcpp/grpcpp.h>
#include "common.pb.h"
#include "edge_service.grpc.pb.h"
#include <sw/redis++/redis++.h>
#include <vector>
#include <queue>
#include <deque>
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
#include "logger_factory.h"


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
		queue_.push_back(value);
		cond_var_.notify_one();
	}

	//пытается извлечь данные. если очередь пуста блокируется до появления данных
	bool pop(T& value);
    bool pop_bulk(std::vector<T>& values, size_t max_count,
                std::chrono::milliseconds tiomeout);

	//сигнализирует о завершении работы очереди
	void shutdown(){
		std::lock_guard<std::mutex> lock(mutex_);
		shutdown_ = true;
		cond_var_.notify_all();
	}

private:
	std::deque<T> queue_;
	mutable std::mutex mutex_;
	std::condition_variable cond_var_;
	bool shutdown_ = false;
};

class EdgeServiceImpl final : public EdgeService::AsyncService{
public:
    explicit EdgeServiceImpl(std::vector<std::unique_ptr<ThreadSafeQueue<SensorData>>>& queues)
    : queues_(queues){}
    
    ~EdgeServiceImpl(){
        //сигнал рабочим потокам завершить работу
        for(auto& q : queues_) {
            q->shutdown();
        }
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
    
    std::vector<std::unique_ptr<ThreadSafeQueue<SensorData>>>& queues_;
};


class EdgeServer {
public:
    EdgeServer()
    :   redis_pool_(std::thread::hardware_concurrency()),
        logger_(iot::logging::LoggerFactory::create_service_logger("edge_server")) {
            size_t num_shards = iot::config::StreamSettings::num_queue_shards;
            queues_.reserve(num_shards);
            for(size_t i =0;i<num_shards;++i){
                queues_.emplace_back(std::make_unique<ThreadSafeQueue<SensorData>>());
            }
            service_ = std::make_unique<EdgeServiceImpl>(queues_);
        }
        
    void Run();
        
private:
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
    std::vector<std::unique_ptr<ThreadSafeQueue<SensorData>>> queues_;
    std::unique_ptr<EdgeServiceImpl> service_;
    std::unique_ptr<Server> server_;
    std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_;
    std::vector<std::jthread> redis_consumers_;
    std::vector<std::jthread> grpc_threads_;
    std::shared_ptr<spdlog::logger> logger_;
        
    void setup_signal_handlers();
    void start_redis_consumer();     
    void start_grpc_server();
    void wait_for_shutdown();
    void cleanup();
    void write_batch_to_redis(const std::vector<SensorData>& batch,
                            std::shared_ptr<sw::redis::Redis>& redis);
    size_t get_get_shrd_index(const std::string& key);
};