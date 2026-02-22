#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_
#include <memory>
#include <string>
#include <unordered_map>
#include "TcpSocket.h"
#include "TcpConnection.h"

class EventLoop;
class Acceptor;

class TcpServer
{
public:
    TcpServer(EventLoop* eventloop);
    ~TcpServer();

    virtual bool Start(std::string ip, uint16_t port);
    virtual void Stop();

    inline std::string GetIPAddress() const{return ip_;}
    inline uint16_t GetPort() const{return port_;}

protected:
    virtual TcpConnection::Ptr OnConnect(int sockfd);
    virtual void AddConnection(int sockfd, TcpConnection::Ptr tcp_cnn);
    virtual void RemoveConnection(int sockfd);

private:
    EventLoop* event_loop_;
    uint16_t port_;
    std::string ip_;
    std::unique_ptr<Acceptor> acceptor_;
    bool is_started_ = false;
    std::unordered_map<int, TcpConnection::Ptr> connections_;
};
#endif