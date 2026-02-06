#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include <cstring>
#include <iostream>

TcpServer::TcpServer(EventLoop *eventloop)
    :event_loop_(eventloop),
    port_(0),
    acceptor_(new Acceptor(eventloop))
{
    acceptor_->SetNewConnectCallBack([this](int fd)
    {
        TcpConnection::Ptr conn = this->OnConnect(fd);
        if(conn)
        {
            this->AddConnection(fd, conn);
            conn->SetDisConnectionCallBack([this](TcpConnection::Ptr conn){
                int fd = conn->GetSocket();
                this->RemoveConnection(fd);
            });
        }
    });
}

TcpServer::~TcpServer()
{
    Stop();
}

bool TcpServer::Start(std::string ip, uint16_t port)
{
    Stop();

    if(!is_started_)
    {
        if(acceptor_->Listen(ip, port) < 0)
        {
            std::cerr << "Listen failed: " << strerror(errno) << std::endl;
            return false;
        }

        port_ = port;
        ip_ = ip;
        is_started_ = true;
    }
    return true; 
}

void TcpServer::Stop()
{
    if(is_started_)
    {
        for(auto iter : connections_)
        {
            iter.second->Disconnect();
        }

        acceptor_->Close();
        is_started_ = false;
    }
}

TcpConnection::Ptr TcpServer::OnConnect(int sockfd)
{
    return std::make_shared<TcpConnection>(event_loop_->GetTaskScheduler().get(), sockfd);
}

void TcpServer::AddConnection(int sockfd, TcpConnection::Ptr tcp_cnn)
{
    connections_.emplace(sockfd, tcp_cnn);
}

void TcpServer::RemoveConnection(int sockfd)
{
    connections_.erase(sockfd);
}
