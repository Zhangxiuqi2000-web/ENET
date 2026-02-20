#ifndef _LOADBANCESERVER_H_
#define _LOADBANCESERVER_H_

#include "../EdoyunNet/TcpServer.h"
#include "define.h"

class LoadBanceServer: public TcpServer, public std::enable_shared_from_this<LoadBanceServer>
{
public:
    static std::shared_ptr<LoadBanceServer> Create(EventLoop* eventloop);  //设单例
    ~LoadBanceServer();

private:
    friend class LoadBanceConnection;
    LoadBanceServer(EventLoop* eventloop);
    virtual TcpConnection::Ptr OnConnect(int socket);
    void UpdateMonitor(const int fd, Monitor_Info* info);
    Monitor_Info* GetMonitorInfo();

private:
    EventLoop* loop_;
    std::mutex mutex_;
    std::map<int, Monitor_Info*> monitorInfos_;
};
#endif