#include "LoginConnection.h"
#include "ORMManager.h"
#include <chrono>

#define TIMEOUT 60

LoginConnection::LoginConnection(TaskScheduler *scheduler, int socket)
    :TcpConnection(scheduler, socket)
{
    this->SetReadCallBack([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });
}

LoginConnection::~LoginConnection()
{
}

bool LoginConnection::IsTimeout(uint64_t timestamp)
{
    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    uint64_t time = nowTimestamp - timestamp;
    return time > TIMEOUT;
}

bool LoginConnection::OnRead(BufferReader &buffer)
{
    if(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

void LoginConnection::HandleMessage(BufferReader &buffer)
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
        HandleLogin((packet_head*)buffer.Peek());
        break;
    case Register:
        HandleRegister((packet_head*)buffer.Peek());
        break;
    case Destroy:
        HandleDestroy((packet_head*)buffer.Peek());
        break;
    default:
        break;
    }
    buffer.Retrieve(head->len);
}

void LoginConnection::Clear()
{
}

void LoginConnection::HandleRegister(const packet_head *data)
{
    RegisterResult reply;
    UserRegister* regi = (UserRegister*)data;
    uint64_t time = regi->timestamp;
    //判断是否已经超时
    if(IsTimeout(time))
    {
        reply.resultCode  = REQUEST_TIMEOUT;
    }
    else
    {
        //判断是否已经注册
        std::string code = regi->GetCode();
        //通过数据库查询code是否存在
        MYSQL_ROW row = ORMManager::GetInstance()->UserLogin(code.c_str());
        if(row == NULL) //未注册
        {
            ORMManager::GetInstance()->UserRegister(regi->GetName().c_str(), regi->GetAcount().c_str(), regi->GetPasswd().c_str(), regi->GetCode().c_str(), "192.168.64.137");
            reply.resultCode = S_OK;
        }
        else    //已注册
        {
            reply.resultCode = ALREADY_REGISTERED;
        }
    }
    this->Send((const char*)&reply, reply.len);
}

void LoginConnection::HandleLogin(const packet_head *data)
{
    LoginResult reply;
    UserLogin* login = (UserLogin*)data;
    uint64_t time = login->timestamp;
    printf("time: %ld\n", time);
    //判断是否已经超时
    if(IsTimeout(time))
    {
        reply.resultCode  = REQUEST_TIMEOUT;
    }
    else
    {
        //判断用户是否存在
        std::string code = login->GetCode();
        //通过数据库查询code是否存在
        MYSQL_ROW row = ORMManager::GetInstance()->UserLogin(code.c_str());
        if(row == NULL)  //未注册
        {
            reply.resultCode = SERVER_ERROR;
        }
        else
        {
            //判断是否已经登陆
            if(atoi(row[4])) 
            {
                printf("online \n");
                reply.resultCode = ALREADY_LOGIN;
            }
            else
            {
                printf("login \n");
                reply.resultCode = S_OK;
                reply.SetIP("192.168.64.137");
                reply.port = 6539;  //信令服务器端口
                printf("RETURN ip:%s\r\n", reply.GetIP().c_str());
                //修改记录
                //先获取当前用户信息
                auto now = std::chrono::system_clock::now();
                auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                ORMManager::GetInstance()->updateClient(row[0], row[1], row[2], row[3], 1, nowTimestamp, "192.168.64.137");
            }
        }
    }
    this->Send((const char*)&reply, reply.len);

}

void LoginConnection::HandleDestroy(const packet_head *data)
{
    UserDestroy* destroy = (UserDestroy*)data;
    ORMManager::GetInstance()->UserDestroy(destroy->GetCode().c_str());
}
