#ifndef _LOADDEFINE_H_
#define _LOADDEFINE_H_
#include <cstdint>
#include <array>
#include <string>
#include "../LoginServer/define.h"

#pragma pack(push,1)

//登陆
struct Login_Info : public packet_head
{
    Login_Info():packet_head()
    {
        cmd = Login;
        len = sizeof(Login_Info);
        timestamp = -1;
    }
    uint64_t timestamp;
};

//登录应答
struct LoginReply : public packet_head
{
    LoginReply():packet_head()
    {
        cmd = Login;  //如果请求超时，将这个cmd置为ERROR
        len = sizeof(LoginReply);
        port = -1;
        ip.fill('\0');
    }
    uint16_t port;
    std::array<char,16> ip;
};

typedef std::pair<int,Monitor_body*> MonitorPair;

struct CmpByValue
{
    bool operator()(const MonitorPair& l, const MonitorPair& r)
    {
        return l.second->mem < r.second->mem;  //按内存从小到大排序
    }
};

#pragma pack(pop)
#endif