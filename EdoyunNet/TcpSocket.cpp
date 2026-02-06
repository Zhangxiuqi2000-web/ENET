#include "TcpSocket.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

//设置非阻塞
void SocketUtil::SetNonBlock(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);  //获取当前属性
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

//设置阻塞
void SocketUtil::SetBlock(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);  //获取当前属性
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));    
}

//地址重用
void SocketUtil::SetReuseAddr(int sockfd)
{
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on));
}

//端口重用
void SocketUtil::SetReusePort(int sockfd)
{
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void*)&on, sizeof(on));
}

//TCP保活
void SocketUtil::SetKeepAlive(int sockfd)
{
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const void*)&on, sizeof(on));
}

//设置发送缓冲区大小
void SocketUtil::SetSendBufSize(int sockfd, int size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const void*)&size, sizeof(size));
}

//设置接收缓冲区大小
void SocketUtil::SetRecvBufSize(int sockfd, int size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const void*)&size, sizeof(size));
}

TcpSocket::TcpSocket()
{
}

TcpSocket::~TcpSocket()
{
}

int TcpSocket::Create()
{
    sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    return sockfd_;
}


bool TcpSocket::Bind(std::string ip, short port)
{
    if (sockfd_ <= 0)  // 确保 sockfd_ 已创建
    {
        std::cerr << "Socket is not created properly." << std::endl;
        return false;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // 使用 inet_pton 替代 inet_addr 来转换 IP
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        return false;
    }

    if (::bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;  // 输出详细错误
        return false;
    }
    return true;
}


// bool TcpSocket::Listen(int backlog)
// {
//     if(sockfd_)
//     {
//         return false;
//     }
//     if(::listen(sockfd_, backlog) == -1)
//     {
//         return false;
//     }
//     return true;
// }

bool TcpSocket::Listen(int backlog)
{
    // 检查 sockfd_ 是否有效，注意你要确保它是一个有效的 socket 描述符。
    if (sockfd_ <= 0)
    {
        std::cerr << "Socket not initialized or invalid file descriptor." << std::endl;
        return false;
    }

    // 调用 listen() 来开始监听
    if (::listen(sockfd_, backlog) == -1)
    {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;  // 输出错误信息
        return false;
    }

    return true;
}

int TcpSocket::Accept()
{
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    return ::accept(sockfd_, (struct sockaddr*)&addr, &addrlen);
}

void TcpSocket::Close()
{
    if(sockfd_ != -1)
    {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

void TcpSocket::ShutdownWrite()
{
    if(sockfd_ != -1)
    {
        shutdown(sockfd_, SHUT_WR);
    }
}
