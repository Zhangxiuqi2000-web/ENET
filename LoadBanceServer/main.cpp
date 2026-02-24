#include <iostream>
#include "../LoginServer/LoginServer.h"
#include "LoadBanceServer.h"
#include "../EdoyunNet/EventLoop.h"

//8523
//启动负载、登陆服务器
int main()
{
    EventLoop loop(1);
    std::shared_ptr<LoginServer> loginServer = nullptr;
    auto loadServer = LoadBanceServer::Create(&loop);
    if(loadServer->Start("192.168.64.137", 8523))
    {
        loginServer = LoginServer::Create(&loop);
        if(loginServer->Start("192.168.64.137", 9867))
        {
            printf("server start successful\n");
        }
    }
    getchar();
    return 0;
}