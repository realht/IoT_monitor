#include "analytics_server.h"

int main(){
    // Отключаем буферизацию
    std::ios::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    AnalyticsServer server;
    server.Run();
    return 0;
}
