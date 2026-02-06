#include <functional>
#include <memory>
#include "Channel.h"
#include "TcpSocket.h"

class EventLoop;

typedef std::function<void(int)> NewConnectCallBack;

class Acceptor
{
public:
    Acceptor(EventLoop* eventloop);
    virtual ~Acceptor();

    void SetNewConnectCallBack(const NewConnectCallBack& cb){new_connection_callback_ = cb;}

    int Listen(std::string ip, uint16_t port);
    void Close();

private:
    void OnAccept();
    EventLoop* event_loop_ = nullptr;
    ChannelPtr channel_ptr_ = nullptr;
    std::unique_ptr<TcpSocket> tcp_socket_;
    NewConnectCallBack new_connection_callback_;
};
