#include "LoadBanceServer.h"
#include "LoadBanceConnection.h"
#include "../EdoyunNet/EventLoop.h"
#include <vector>
#include <algorithm>

std::shared_ptr<LoadBanceServer> LoadBanceServer::Create(EventLoop *eventloop)
{
    std::shared_ptr<LoadBanceServer> server(new LoadBanceServer(eventloop));
    return server;
}

LoadBanceServer::LoadBanceServer(EventLoop* eventloop)
    :TcpServer(eventloop)
    ,loop_(eventloop)
{

}

LoadBanceServer::~LoadBanceServer()
{
    for(auto iter: monitorInfos_)
    {
        if(iter.second)
        {
            delete iter.second;
            iter.second = nullptr;
        }
    }
}

TcpConnection::Ptr LoadBanceServer::OnConnect(int socket)
{
    return std::make_shared<LoadBanceConnection>(shared_from_this(), loop_->GetTaskScheduler().get(), socket);
}

void LoadBanceServer::UpdateMonitor(const int fd, Monitor_Info *info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    //更新资源
    monitorInfos_[fd] = info;
}

Monitor_Info *LoadBanceServer::GetMonitorInfo()
{
    //排序
    std::lock_guard<std::mutex> lock(mutex_);
    //先将map中元素转到vector中
    std::vector<MonitorPair> vec(monitorInfos_.begin(), monitorInfos_.end());
    sort(vec.begin(), vec.end(), CmpByValue());  //从小到大排序，第一个即是最优服务
    return vec[0].second;
}
