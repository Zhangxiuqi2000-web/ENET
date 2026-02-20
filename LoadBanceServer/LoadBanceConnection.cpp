#include "LoadBanceConnection.h"
#include <chrono>

#define TIMEOUT 60
LoadBanceConnection::LoadBanceConnection(std::shared_ptr<LoadBanceServer> loadbance_server, TaskScheduler *task_scheduler, int sockfd)
    :TcpConnection(task_scheduler, sockfd)
    ,loadbance_server_(loadbance_server)
    ,socket_(sockfd)
{
    this->SetReadCallBack([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });

    this->SetCloseCallBack([this](std::shared_ptr<TcpConnection> conn){
        return this->Disconnection();
    });
}

LoadBanceConnection::~LoadBanceConnection()
{
}

void LoadBanceConnection::Disconnection()
{
}

bool LoadBanceConnection::OnRead(BufferReader &buffer)
{
    if(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

bool LoadBanceConnection::IsTimeout(uint64_t timestamp)
{
    //获取当前时间
    auto now = std::chrono::system_clock::now();
    auto nowtimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    //计算差值
    uint64_t time = nowtimestamp - timestamp;
    return time > TIMEOUT;
}

void LoadBanceConnection::HandleMessage(BufferReader &buffer)
{
    if(buffer.ReadableBytes() < sizeof(packet_head))
    {
        return;
    }
    packet_head* head = (packet_head*)buffer.Peek();
    if(buffer.ReadableBytes() < head->len)
    {
        return;
    }

    switch(head->cmd)
    {
    case Login:
        HandleLogin(buffer);
        break;
    case Monitor:
        HandleMonitorInfo(buffer);
        break;
    default:
        printf("cmd error\n");
        break;
    }
    buffer.Retrieve(head->len);
}

void LoadBanceConnection::HandleLogin(BufferReader &buffer)
{
    LoginReply reply;
    Login_Info* info = (Login_Info*)buffer.Peek();
    if(IsTimeout(info->timestamp))  //超时报错
    {
        reply.cmd = Error;
    }
    else
    {
        //获取IP和端口
        auto server = loadbance_server_.lock();
        if(server)
        {
            Monitor_Info* monitor = server->GetMonitorInfo(); //在函数中做资源管理算法来分配
            reply.ip = monitor->ip;
            reply.port = monitor->port;
        }
        else
        {
            reply.cmd = Error;
        }
    }
    Send((const char*)&reply, reply.len);
}

void LoadBanceConnection::HandleMonitorInfo(BufferReader &buffer)
{
    //处理心跳
    Monitor_Info* body = (Monitor_Info*)buffer.Peek();
    auto server = loadbance_server_.lock();
    if(server)
    {
        server->UpdateMonitor(socket_, body);
    }
}
