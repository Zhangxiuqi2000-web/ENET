#ifndef _LOADDEFINE_H_
#define _LOADDEFINE_H_
#include <cstdint>
#include <array>
#include <string>

enum Cmd: uint16_t
{
    Monitor,
    Error,
    Login,
};

#pragma pack(push,1)
//包头
struct packet_head
{
    packet_head(): len(-1), cmd(-1){}
    uint16_t len;
    uint16_t cmd;
};

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

//资源监测
struct Monitor_Info: public packet_head
{
    Monitor_Info(): packet_head()
    {
        cmd = Monitor;
        len = sizeof(Monitor_Info);
        mem = -1;
        port = -1;
        ip.fill('\0');
    }
    void SetIP(const std::string& str)
    {
        str.copy(ip.data(), ip.size());
    }
    std::string GetIP()
    {
        return std::string(ip.data());
    }
    uint8_t mem;  //内存使用百分比，取整
    uint16_t port;
    std::array<char, 16> ip;

};
typedef std::pair<int,Monitor_Info*> MonitorPair;

struct CmpByValue
{
    bool operator()(const MonitorPair& l, const MonitorPair& r)
    {
        return l.second->mem < r.second->mem;  //按内存从小到大排序
    }
};

#pragma pack(pop)
#endif