#include "SigConnection.h"
#include "ConnectionManager.h"
#include <algorithm>

SigConnection::SigConnection(TaskScheduler *scheduler, int socket)
    :TcpConnection(scheduler, socket)
    ,state_(NONE)
{
    //设置读回调函数处理读数据
    this->SetReadCallBack([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });

    //设置关闭回调释放资源
    this->SetCloseCallBack([this](std::shared_ptr<TcpConnection> conn){
        this->Disconnect();
    });
}

SigConnection::~SigConnection()
{
    Clear();
}

void SigConnection::DisConnected()
{
    Clear();
}

void SigConnection::AddCustom(const std::string &code)
{
    //添加客户，可能是拉流器可能是推流器
    for(const auto idefy : objects_)
    {
        if(idefy == code)  //目标客户端已添加
        {
            return;
        }
    }
    objects_.push_back(code);
}

void SigConnection::RemoveCustom(const std::string &code)
{
    if(objects_.empty())
    {
        return;
    }
    objects_.erase(std::remove(objects_.begin(), objects_.end(), code), objects_.end());
    if(objects_.empty())
    {
        state_ == IDLE;  //说明当前没有控制端，自己也不是
    }
}

bool SigConnection::OnRead(BufferReader &buffer)
{
    if(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

void SigConnection::HandleMessage(BufferReader &buffer)
{
    if(buffer.ReadableBytes() < sizeof(packet_head))
    {
        //数据不完整
        return;
    }
    packet_head* data = (packet_head*)buffer.Peek();
    if(buffer.ReadableBytes() < data->len)  //数据不完整
    {
        return;
    }

    switch (data->cmd)
    {
    case JOIN:
        HandleJoin(data);
        break;
    case OBTAINSTREAM:
        HandleObtainStream(data);
        break;
    case CREATESTREAM:
        HandleCreateStream(data);
        break;
    case DELETESTREAM:
        HandleDeleteStream(data);
        break;
    case MOUSE:
    case MOUSEMOVE:
    case KEY:
    case WHEEL:
        HandleOtherMessage(data);
        break;
    }
    //更新缓冲区
    buffer.Retrieve(data->len);

}

void SigConnection::HandleObtainStream(const packet_head *data)
{
    ObtainStream_body* body = (ObtainStream_body*)data;
    return this->DoObtainStream(body);
}

void SigConnection::HandleJoin(const packet_head *data)
{
    JoinReply_body reply_body;  //准备一个应答
    Join_body* body = (Join_body*)data;
    if(this->IsNoJoin())
    {
        std::string code = body->GetId();
        TcpConnection::Ptr ptr = ConnectionManager::GetInstance()->QueryConn(code);  //是不是已经存在，若存在则报错
        if(ptr)
        {
            reply_body.SetCode(ERROR);
            this->Send((const char*)&reply_body, reply_body.len);
            return;
        }
        code_ = code;
        state_ = IDLE;
        ConnectionManager::GetInstance()->AddConn(code_, shared_from_this());
        printf("join count: %d\n", ConnectionManager::GetInstance()->Size());
        reply_body.SetCode(SUCCESSFUL);
        this->Send((const char*)&reply_body, reply_body.len);
        return;
    }

    //已经创建
    reply_body.SetCode(ERROR);
    this->Send((const char*)&reply_body, reply_body.len);
}

void SigConnection::Clear()
{
    state_ = CLOSE;
    conn_ = nullptr;
    DeleteStream_body body;
    for(auto iter : objects_)
    {
        TcpConnection::Ptr con = ConnectionManager::GetInstance()->QueryConn(iter);
        if(con)
        {
            auto ctrCon = std::dynamic_pointer_cast<SigConnection>(con);
            if(ctrCon)
            {
                ctrCon->RemoveCustom(code_);
            }
            body.SetStreamCount(ctrCon->objects_.size());
            con->Send((const char*)&body, body.len);
        }
    }
    objects_.clear();
    ConnectionManager::GetInstance()->RemoveConn(code_);
    printf("con size: %d\n", ConnectionManager::GetInstance()->Size());
}

void SigConnection::HandleCreateStream(const packet_head *data)
{
    CreateStream_body* body = (CreateStream_body*)data;
    return this->DoCreateStream(body);
}

void SigConnection::HandleDeleteStream(const packet_head *data)
{
    if(this->IsBusy())
    {
        Clear();
    }
}

void SigConnection::HandleOtherMessage(const packet_head *data)
{
    //鼠标、键盘消息，转发
    if(conn_ && state_ == PULLER)  //说明当前在拉流
    {
        conn_->Send((const char*)data, data->len);
    }
}

void SigConnection::DoObtainStream(const packet_head *data)
{
    ObtainStreamReply_body reply;
    CreateStream_body create_reply;
    std::string code = ((ObtainStream_body*)data)->GetId();
    TcpConnection::Ptr conn = ConnectionManager::GetInstance()->QueryConn(code);
    if(!conn)
    {
        printf("远程目标不存在\n");
        reply.SetCode(ERROR);
        this->Send((const char*)&reply, reply.len);
        return;
    }
    if(conn == shared_from_this())  //不能控制自己
    {
        printf("不能控制自己\n");
        reply.SetCode(ERROR);
        this->Send((const char*)&reply, reply.len);
        return;
    }
    if(this->IsIdle())  //若是空闲状态则去获取流
    {
        auto con = std::dynamic_pointer_cast<SigConnection>(conn);
        switch(con->GetRoleState())
        {
        case IDLE:  //目标空闲，通知他去推流
            printf("目标空闲\n");
            this->state_ = PULLER;
            this->AddCustom(code);  //添加被控端
            con->AddCustom(code_);  //被控端需要添加控制端
            reply.SetCode(SUCCESSFUL);
            conn_ = conn;  //我们就可以通过被控端来转发消息
            conn->Send((const char*)&create_reply, create_reply.len);
            break;
        case NONE:
            printf("目标没上线\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&create_reply, create_reply.len);
            break;
        case CLOSE:
            printf("目标离线\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&create_reply, create_reply.len);
            break;
        case PULLER:  //目标拉流（控制端），控制端不能控制控制端
            printf("目标忙碌\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&create_reply, create_reply.len);
            break;
        case PUSHER:  //目标推流（被控端），我们可以去拉流
            if(con->GetStreamAddress().empty())
            {
                printf("目标正在推流，但是流地址异常\n");
                reply.SetCode(ERROR);
                this->Send((const char*)&create_reply, create_reply.len);
            }
            else
            {
                printf("目标正在推流\n");
                this->state_ = PULLER;
                this->AddCustom(code);
                con->AddCustom(code_);
                //在推流，流已经存在，不需要重新创建流，只需要播放流
                PlayStream_body play_body;
                play_body.SetCode(SUCCESSFUL);
                play_body.SetstreamAddres(con->GetStreamAddress());
                this->Send((const char*)&play_body, play_body.len);
            }
            break;
        }
    }
    else  //本身是忙碌
    {
        reply.SetCode(ERROR);
        this->Send((const char*)&create_reply, create_reply.len);
    }
}

void SigConnection::DoCreateStream(const packet_head *data)
{
    PlayStream_body body;
    //判断所有连接的状态，如果连接器是空闲，我们去回应
    for(auto idefy : objects_)
    {
        TcpConnection::Ptr conn = ConnectionManager::GetInstance()->QueryConn(idefy);
        if(!conn)
        {
            this->RemoveCustom(idefy);
            continue;
        }
        auto con = std::dynamic_pointer_cast<SigConnection>(conn);
        if(streamAddress_.empty())//流地址异常
        {
            printf("流地址异常\n");
            con->state_ = IDLE;
            body.SetCode(ERROR);
            con->Send((const char*)&body,body.len);
            continue;
        }
        switch (con->GetRoleState())
        {
        case NONE:
        case IDLE:
        case CLOSE:
        case PUSHER:
            body.SetCode(ERROR);
            this->RemoveCustom(con->GetCode());
            con->Send((const char*)&body,body.len);
            break;
        case PULLER:
        this->state_ = PUSHER;
        body.SetCode(SUCCESSFUL);
        body.SetstreamAddres(streamAddress_);
        printf("streamadder: %s\n",streamAddress_.c_str());
        conn->Send((const char*)&body,body.len);
        break;
        default:
            break;
        }
    }
}
