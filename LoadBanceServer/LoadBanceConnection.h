#ifndef _LOADBANCECONNECTION_H_
#define _LOADBANCECONNECTION_H_

#include "../EdoyunNet/TcpConnection.h"
#include "LoadBanceServer.h"

class LoadBanceConnection: public TcpConnection
{
public:
    LoadBanceConnection(std::shared_ptr<LoadBanceServer> loadbance_server, TaskScheduler* task_scheduler, int sockfd);
    ~LoadBanceConnection();

protected:
    void Disconnection();
    bool OnRead(BufferReader& buffer);
    bool IsTimeout(uint64_t timestamp);
    void HandleMessage(BufferReader& buffer);
    void HandleLogin(BufferReader& buffer);
    void HandleMonitorInfo(BufferReader& buffer);

private:
    int socket_;
    std::weak_ptr<LoadBanceServer> loadbance_server_;
};
#endif