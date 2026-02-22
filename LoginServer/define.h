#ifndef _DEFINE_H_
#define _DEFINE_H_
#include <cstdint>
#include <string>
#include <array>
#include <sys/sysinfo.h>
enum Cmd : uint16_t
{
    Monitor,
    ERROR,
    Login,
    Register,
    Destroy,
};

enum ResultCode
{
    S_OK = 0,
    SERVER_ERROR,
    REQUEST_TIMEOUT,
    ALREADY_REGISTERED,
    USER_DISAPPEAR,
    ALREADY_LOGIN,
    VERFICATE_FAILED
};

struct packet_head {
    packet_head()
        :len(-1)
        , cmd(-1) {}
    uint16_t len;
    uint16_t cmd;
};

struct UserRegister: public packet_head
{
    UserRegister(): packet_head()
    {
        cmd = Register;

    }

    void SetName(const std::string& str)
    {
        str.copy(name.data(), name.size(), 0);
    }
    std::string GetName(){
        return std::string(name.data());
    }
    void SetCode(const std::string& str)
    {
        str.copy(code.data(), code.size(), 0);
    }
    std::string GetCode(){
        return std::string(code.data());
    }
    void SetAcount(const std::string& str)
    {
        str.copy(acount.data(), acount.size(), 0);
    }
    std::string GetAcount(){
        return std::string(acount.data());
    }
    void SetPasswd(const std::string& str)
    {
        str.copy(passwd.data(), passwd.size(), 0);
    }
    std::string GetPasswd(){
        return std::string(passwd.data());
    }

    uint16_t cmd;
    std::array<char, 20> name;
    std::array<char, 20> code;
    std::array<char, 12> acount;
    std::array<char, 20> passwd;
    uint64_t timestamp;
};

struct UserLogin: public packet_head
{
    UserLogin(): packet_head()
    {
        cmd = Login;
    }

    void SetCode(const std::string& str)
    {
        str.copy(code.data(), code.size(), 0);
    }
    std::string GetCode(){
        return std::string(code.data());
    }
    void SetAcount(const std::string& str)
    {
        str.copy(acount.data(), acount.size(), 0);
    }
    std::string GetAcount(){
        return std::string(acount.data());
    }
    void SetPasswd(const std::string& str)
    {
        str.copy(passwd.data(), passwd.size(), 0);
    }
    std::string GetPasswd(){
        return std::string(passwd.data());
    }

    uint16_t cmd;
    std::array<char, 20> code;
    std::array<char, 12> acount;
    std::array<char, 33> passwd;  //MD5
    uint64_t timestamp;
};



struct UserDestroy: public packet_head
{
    UserDestroy(): packet_head()
    {
        cmd = Destroy;
    }

    void SetCode(const std::string& str)
    {
        str.copy(code.data(), code.size(), 0);
    }
    std::string GetCode(){
        return std::string(code.data());
    }

    uint16_t cmd;
    std::array<char, 20> code;
};

struct RegisterResult: public packet_head
{
    RegisterResult(): packet_head()
    {
        cmd = Register;
        len = sizeof(RegisterResult);
    }
    ResultCode resultCode;
};

struct LoginResult: public packet_head
{
    LoginResult(): packet_head()
    {
        cmd = Login;
        len = sizeof(LoginResult);
    }

    void SetIP(const std::string& str)
    {
        str.copy(ctrSvrIP.data(), ctrSvrIP.size(), 0);
    }
    std::string GetIP(){
        return std::string(ctrSvrIP.data());
    }

    ResultCode resultCode;
    uint16_t port;
    std::array<char, 16> ctrSvrIP;
};

struct Monitor_body : public packet_head {
    Monitor_body()
        :packet_head()
    {
        cmd = Monitor;
        len = sizeof(Monitor_body);
        ip.fill('\0');
    }
    void SetIp(const std::string& str)
    {
        str.copy(ip.data(), ip.size(), 0);
    }
    std::string GetIp()
    {
        return std::string(ip.data());
    }
    uint8_t mem;
    std::array<char, 16> ip;
    uint16_t port;
};
#endif