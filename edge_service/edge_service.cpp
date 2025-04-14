#include "edge_server.h"

int main(){
	// Отключаем буферизацию
    std::ios::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

	std::cout << "Greating to async grpc edge-server\n";
	EdgeServer server;
	server.Run();
	
	std::cout << "Server shutdown gracefully!\n";
	return 0;	
	}
