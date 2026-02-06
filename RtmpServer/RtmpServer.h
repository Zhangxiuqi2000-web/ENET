#include <mutex>
#include "../EdoyunNet/TcpServer.h"
#include "rtmp.h"
#include "RtmpSession.h"

class RtmpServer: public TcpServer, public Rtmp, public std::enable_shared_from_this<RtmpServer>
{
public:
    using EventCallBack = std::function<void(std::string type, std::string streampath)>;

    //创建服务器
    static std::shared_ptr<RtmpServer> Create(EventLoop* eventloop);  //单例，构造函数设为私有
    ~RtmpServer();
    void SetEventCallBack(const EventCallBack cb);

private:
    friend class RtmpConnection;
    
    RtmpServer(EventLoop* eventloop);
    void AddSession(std::string stream_path); //每个session由流路径区分
    void RemoveSession(std::string stream_path);

    RtmpSession::Ptr GetSession(std::string stream_path);
    
    bool HasPublisher(std::string stream_path);
    bool HasSession(std::string stream_path);
    void NotifyEvent(std::string type, std::string stream_path);

    virtual TcpConnection::Ptr OnConnect(int socket);

    EventLoop* loop_;
    std::mutex mutex_;
    std::unordered_map<std::string, RtmpSession::Ptr> rtmp_sessions_;
    std::vector<EventCallBack> event_callbacks_;
};