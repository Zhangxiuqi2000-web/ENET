#include <iostream>
#include "../EdoyunNet/EventLoop.h"
#include "SigServer.h"

int main()
{
    int count = std::thread::hardware_concurrency();
    EventLoop loop(2);
    auto sig_server = SigServer::Create(&loop);

    if(!sig_server->Start("192.168.64.136", 6539))
    {
        printf("sig server failed\n");
    }
    printf("sig server success\n");

    //getchar();
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}