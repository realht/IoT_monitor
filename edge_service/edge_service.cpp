#include <iostream>
#include <grpcpp/grpcpp.h>
#include "edge_service.grpc.pb.h"
#include <sw/redis++/redis++.h>
//#include <sw/redis++/async_redis++.h>
#include <vector>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;
using iot::v1::SensorData;
using iot::v1::Ack;
using iot::v1::EdgeService;

class EdgeServiceImpl final : public EdgeService::Service{
public:

	EdgeServiceImpl(){
		redis_= std::make_unique<sw::redis::Redis>("tcp://localhost:6379");
		// Проверка подключения к Redis
        try {
            redis_->ping();
            std::cout << "Connected to Redis successfully\n";
        } catch (const std::exception &e) {
            std::cerr << "Redis connection error: " << e.what() << "\n";
            exit(1);
        }
	}

	Status StreamTelemetry(ServerContext* context, 
		ServerReader<SensorData>* reader, 
		Ack* ack) override{
			SensorData data;
			while(reader->Read(&data)){
				//write to console for test only
				std::cout << "Recivind data from device " << data.device_id() <<
				": Temperature=" << data.temperature() << ", Humidity=" <<
				data.humidity() << '\n';
				
				//через передачу данных из вектора
				std::vector<std::pair<std::string, std::string_view>> entires{
					{"device_id", data.device_id()},
					{"temperature", std::to_string(data.temperature())},
					{"humidity", std::to_string(data.humidity())},
					{"timestamp", std::to_string(data.timestamp().seconds())}};
				redis_->xadd(
					"telemetry_stream",
					"*",
					entires.begin(), entires.end());

				redis_->expire("telemetry_stream",3600*24);
				
				//optimize data for cashing
				//нужно будет написать потом


			}
			ack->set_success(true);
			return Status::OK;
		}
private:
	std::unique_ptr<sw::redis::Redis> redis_;
};

void RunServer(){
	std::string server_address("0.0.0.0:50051");
	EdgeServiceImpl service;
	ServerBuilder builder;
	builder.AddListeningPort(server_address,grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Edge server listining on " << server_address << '\n';
	server->Wait();
}

int main(){
	std::cout << "Greating to sync grpc req server\n";
	RunServer();
	return 0;	
	}
