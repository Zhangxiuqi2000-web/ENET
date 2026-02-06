#include <iostream>
#include "EventLoop.h"
#include "TcpServer.h"

int main()
{
    uint32_t count = std::thread::hardware_concurrency();
    EventLoop event_loop(count);
    TcpServer* server = new TcpServer(&event_loop);
    if(!server->Start("0.0.0.0", 5489))
    {
        std::cerr << "Server Start Failed" << std::endl;
        return -1;
    }
    std::cout << "Server Start" << std::endl;
    getchar();
    server->Stop();
    std::cout << "Server Terminate" << std::endl;
    return 0;
}