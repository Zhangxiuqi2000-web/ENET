#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Channel.h"
#include "TcpSocket.h"
#include "TaskScheduler.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    using DisConnectCallBack = std::function<void(std::shared_ptr<TcpConnection>)>;
    using CloseCallBack = std::function<void(std::shared_ptr<TcpConnection>)>;
    using ReadCallBack = std::function<bool(std::shared_ptr<TcpConnection>, BufferReader& buffer)>;
    TcpConnection(TaskScheduler* task_scheduler, int sockfd);
    ~TcpConnection();

    inline TaskScheduler* GetTaskScheduler()const{return task_scheduler_;}
    inline void SetReadCallBack(const ReadCallBack& cb){readCB_ = cb;}
    inline void SetCloseCallBack(const CloseCallBack& cb){closeCB_ = cb;} 
    inline void SetDisConnectionCallBack(const DisConnectCallBack& cb){disconnectCB_ = cb;}
    inline bool IsClosed()const{return is_closed_;}
    inline int GetSocket()const{return channel_->GetSocket();}

    void Send(std::shared_ptr<char> data, uint32_t size);
    void Send(const char* data, uint32_t size);
    void Disconnect();

protected:
    virtual void HandleRead();
    virtual void HandleWrite();
    virtual void HandleClose();
    virtual void HandleError();

protected:
    TaskScheduler* task_scheduler_;

    std::unique_ptr<BufferReader> read_buffer_;
    std::unique_ptr<BufferWriter> write_buffer_;
    bool is_closed_;

private:
    void Close(); 
    std::mutex mutex_;
    std::shared_ptr<Channel> channel_ = nullptr;
    DisConnectCallBack disconnectCB_;
    CloseCallBack closeCB_;
    ReadCallBack readCB_;
};
#endif