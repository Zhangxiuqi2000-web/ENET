#include "Acceptor.h"
#include "EventLoop.h"
#include <iostream>
#include <cstring>

Acceptor::Acceptor(EventLoop *eventloop)
    :event_loop_(eventloop),
    tcp_socket_(new TcpSocket())
{
}

Acceptor::~Acceptor()
{
}

int Acceptor::Listen(std::string ip, uint16_t port)
{
    if(tcp_socket_->GetSocket() > 0)
    {
        tcp_socket_->Close();
    }
    int fd = tcp_socket_->Create();
    channel_ptr_.reset(new Channel(fd));
    SocketUtil::SetNonBlock(fd);
    SocketUtil::SetReuseAddr(fd);
    SocketUtil::SetReusePort(fd);

    if(!tcp_socket_->Bind(ip, port))
    {
        std::cerr << "Bind failed for IP: " << ip << " and Port: " << port << std::endl;
        return -1;
    }

    if(!tcp_socket_->Listen(1024))
    {
        std::cerr << "Listen failed for IP: " << ip << " and Port: " << port << std::endl;
        return -2;
    }
    
    channel_ptr_->SetReadCallback([this](){this->OnAccept();});
    channel_ptr_->EnableReading();
    event_loop_->UpdateChannel(channel_ptr_);
    return 0;
}

void Acceptor::Close()
{
    if(tcp_socket_->GetSocket() > 0)
    {
        event_loop_->RemoveChannel(channel_ptr_);
        tcp_socket_->Close();
    }
}

// void Acceptor::OnAccept()
// {
//     int fd = tcp_socket_->Accept();
//     if(fd > 0)
//     {
//         if(new_connection_callback_)
//         {
//             new_connection_callback_(fd);
//         }
//     }
// }

void Acceptor::OnAccept()
{
    int fd = tcp_socket_->Accept();
    if (fd > 0)
    {
        //std::cout << "New connection accepted, fd: " << fd << std::endl;
        if (new_connection_callback_)
        {
            new_connection_callback_(fd);
        }
    }
    else
    {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;  // 输出具体错误信息
    }
}

