#include "ConnectionManager.h"

std::unique_ptr<ConnectionManager> ConnectionManager::instance_ = nullptr;

ConnectionManager::ConnectionManager()
{
}

ConnectionManager::~ConnectionManager()
{
    Close();
}

ConnectionManager *ConnectionManager::GetInstance()
{
    static std::once_flag flag;
    std::call_once(flag, [&](){
        instance_.reset(new ConnectionManager());  //只会使用一次
    });
    return nullptr;
}

void ConnectionManager::AddConn(const std::string &idefy, const TcpConnection::Ptr conn)
{
    if(idefy.empty())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it == connMaps_.end())
    {
        connMaps_.emplace(idefy, conn);
    }
}

void ConnectionManager::RemoveConn(const std::string &idefy)
{
    if(idefy.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it != connMaps_.end())
    {
        connMaps_.erase(idefy);
    }
}

TcpConnection::Ptr ConnectionManager::QueryConn(const std::string &idefy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it != connMaps_.end())
    {
        return it->second;
    }
    return nullptr;
}

void ConnectionManager::Close()
{
    connMaps_.clear();
}
