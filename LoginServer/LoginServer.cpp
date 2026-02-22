#include "LoginServer.h"
#include "../EdoyunNet/EventLoop.h"
#include "LoginConnection.h"

std::shared_ptr<LoginServer> LoginServer::Cteate(EventLoop *eventloop)
{
    std::shared_ptr<LoginServer> server(new LoginServer(eventloop));
    return server;
}

LoginServer::LoginServer(EventLoop* eventloop)
    :TcpServer(eventloop)
    ,loop_(eventloop)
    ,client_(nullptr)
{
    client_->Create();
    if(client_->Connect("192.168.44.130", 8523))  //连接负载服务器
    {
        id_ = loop_->AddTimer([this](){
            client_->getMonitorInfo();
            return true;
        }, 50000);
    }
}

LoginServer::~LoginServer()
{
}

TcpConnection::Ptr LoginServer::OnConnect(int socket)
{
    return std::make_shared<LoginConnection>(loop_->GetTaskScheduler().get(), socket);
}
