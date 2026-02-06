#include "TcpConnection.h"
#include <unistd.h>

TcpConnection::TcpConnection(TaskScheduler *task_scheduler, int sockfd)
    :task_scheduler_(task_scheduler),
    read_buffer_(new BufferReader()),
    write_buffer_(new BufferWriter(500)),
    channel_(new Channel(sockfd))
{
    is_closed_ = false;

    // //心跳数据
    // task_scheduler_->AddTimer([this](){
    //     char buffer[] = "Hello, I'm Server.";
    //     this->Send(buffer, sizeof(buffer));
    //     return true;}, 1000);

    channel_->SetReadCallback([this](){this->HandleRead();});
    channel_->SetWriteCallback([this](){this->HandleWrite();});
    channel_->SetCloseCallback([this](){this->HandleClose();});
    channel_->SetErrorCallback([this](){this->HandleError();});

    //设置套接字属性
    SocketUtil::SetNonBlock(sockfd);
    SocketUtil::SetSendBufSize(sockfd, 100 * 1024);
    SocketUtil::SetKeepAlive(sockfd);

    //设置关心事件
    channel_->EnableReading();
    task_scheduler_->UpdateChannel(channel_);
}

TcpConnection::~TcpConnection()
{
    int fd = channel_->GetSocket();
    if(fd > 0)
    {
        ::close(fd);
    }
}

void TcpConnection::Send(std::shared_ptr<char> data, uint32_t size)
{
    if(!is_closed_)
    {
        mutex_.lock();
        write_buffer_->Append(data, size);
        mutex_.unlock();
        this->HandleWrite();
    }
}

void TcpConnection::Send(const char *data, uint32_t size)
{
    if(!is_closed_)
    {
        write_buffer_->Append(data, size);
        this->HandleWrite();
    }
}

void TcpConnection::Disconnect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::HandleRead()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_closed_)
        {
            return;
        }
        int ret = read_buffer_->read(channel_->GetSocket());
        if (ret < 0)
        {
            this->Close();
            return;
        }
    }
    if(readCB_)
    {
        bool ret = readCB_(shared_from_this(), *read_buffer_);
        if(!ret)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            this->Close();
        }
    }

    // //回声
    // std::string data;
    // uint32_t size = read_buffer_->ReadAll(data);
    // if(size)
    // {
    //     this->Send(data.data(), data.size());
    // }
}

void TcpConnection::HandleWrite()
{
    if(is_closed_)
    {
        return;
    }
    if(!mutex_.try_lock())
    {
        return;
    }
    int ret = 0;
    bool empty = false;
    do{
        ret = write_buffer_->Send(channel_->GetSocket());
        if(ret < 0)
        {
            this->Close();
            mutex_.unlock();
            return;
        }
        empty = write_buffer_->IsEmpty();
    }while(0);

    if(empty)
    {
        if(channel_->IsWriting())
        {
            channel_->DisableWriting();
            task_scheduler_->UpdateChannel(channel_);
        }
    }
    else if(!channel_->IsWriting())
    {
        channel_->EnableWriting();
        task_scheduler_->UpdateChannel(channel_);
    }
    mutex_.unlock();
}

void TcpConnection::HandleClose()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::HandleError()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::Close()
{
    if(!is_closed_)
    {
        is_closed_ = true;
        task_scheduler_->RemoveChannel(channel_);
        if(closeCB_)
        {
            closeCB_(shared_from_this());
        }
        if(disconnectCB_)
        {
            disconnectCB_(shared_from_this());
        }
    }
}
