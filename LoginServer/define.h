#ifndef _DEFINE_H_
#define _DEFINE_H_
#include <cstdint>
#include <string>
#include <array>
#include <string.h>
#include <sys/sysinfo.h>
#pragma pack(push,1)
enum Cmd : uint16_t
{
    Minotor = 0,
    Monitor = Minotor,
    ERROR = 1,
    Login = 2,
    Register = 3,
    Destory = 4,
    Destroy = Destory,
};

enum ResultCode
{
    S_OK = 0,
    SERVER_ERROR = 1,
    REQUEST_TIMEOUT = 2,
    ALREADY_REDISTERED = 3,
    ALREADY_REGISTERED = ALREADY_REDISTERED,
    USER_DISAPPEAR = 4,
    ALREADY_LOGIN = 5,
    VERFICATE_FAILED = 6
};

struct packet_head {
    packet_head()
        :len(-1)
        , cmd(-1) {}
    uint16_t len;
    uint16_t cmd;
    static size_t SafeLen(const char* data, size_t maxLen)
    {
        size_t len = 0;
        while(len < maxLen && data[len] != '\0')
        {
            ++len;
        }
        return len;
    }
};

struct UserRegister: public packet_head
{
    UserRegister(): packet_head()
    {
        cmd = Register;
        len = sizeof(UserRegister);
        code.fill('\0');
        name.fill('\0');
        count.fill('\0');
        passwd.fill('\0');
    }

    void SetCode(const std::string& str)
    {
        code.fill('\0');
        str.copy(code.data(), code.size() - 1, 0);
        code.back() = '\0';
    }
    std::string GetCode(){
        return std::string(code.data(), SafeLen(code.data(), code.size()));
    }
    void SetName(const std::string& str)
    {
        name.fill('\0');
        str.copy(name.data(), name.size() - 1, 0);
        name.back() = '\0';
    }
    std::string GetName(){
        return std::string(name.data(), SafeLen(name.data(), name.size()));
    }
    void SetCount(const std::string& str)
    {
        count.fill('\0');
        str.copy(count.data(), count.size() - 1, 0);
        count.back() = '\0';
    }
    std::string GetCount(){
        return std::string(count.data(), SafeLen(count.data(), count.size()));
    }
    void SetAcount(const std::string& str)
    {
        SetCount(str);
    }
    std::string GetAcount(){
        return GetCount();
    }
    void SetPasswd(const std::string& str)
    {
        passwd.fill('\0');
        str.copy(passwd.data(), passwd.size() - 1, 0);
        passwd.back() = '\0';
    }
    std::string GetPasswd(){
        return std::string(passwd.data(), SafeLen(passwd.data(), passwd.size()));
    }

    std::array<char, 20> code;
    std::array<char, 20> name;
    std::array<char, 12> count;
    std::array<char, 20> passwd;
    uint64_t timestamp;
};

struct UserLogin: public packet_head
{
    UserLogin(): packet_head()
    {
        cmd = Login;
        len = sizeof(UserLogin);
        code.fill('\0');
        count.fill('\0');
        passwd.fill('\0');
    }

    void SetCode(const std::string& str)
    {
        code.fill('\0');
        str.copy(code.data(), code.size() - 1, 0);
        code.back() = '\0';
    }
    std::string GetCode(){
        return std::string(code.data(), SafeLen(code.data(), code.size()));
    }
    void SetCount(const std::string& str)
    {
        count.fill('\0');
        str.copy(count.data(), count.size() - 1, 0);
        count.back() = '\0';
    }
    std::string GetCount(){
        return std::string(count.data(), SafeLen(count.data(), count.size()));
    }
    void SetAcount(const std::string& str)
    {
        SetCount(str);
    }
    std::string GetAcount(){
        return GetCount();
    }
    void SetPasswd(const std::string& str)
    {
        passwd.fill('\0');
        str.copy(passwd.data(), passwd.size() - 1, 0);
        passwd.back() = '\0';
    }
    std::string GetPasswd(){
        return std::string(passwd.data(), SafeLen(passwd.data(), passwd.size()));
    }

    std::array<char, 20> code;
    std::array<char, 12> count;
    std::array<char, 33> passwd;  //MD5
    uint64_t timestamp;
};



struct UserDestroy: public packet_head
{
    UserDestroy(): packet_head()
    {
        cmd = Destroy;
        len = sizeof(UserDestroy);
        code.fill('\0');
    }

    void SetCode(const std::string& str)
    {
        code.fill('\0');
        str.copy(code.data(), code.size() - 1, 0);
        code.back() = '\0';
    }
    void SeCode(const std::string& str)
    {
        SetCode(str);
    }
    std::string GetCode(){
        return std::string(code.data(), SafeLen(code.data(), code.size()));
    }

    std::array<char, 20> code;
};

using UserDestory = UserDestroy;

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
        ctrSvrIP.fill('\0');
        str.copy(ctrSvrIP.data(), ctrSvrIP.size() - 1, 0);
        ctrSvrIP.back() = '\0';
    }
    void SetIp(const std::string& str)
    {
        SetIP(str);
    }
    std::string GetIP(){
        return std::string(ctrSvrIP.data(), SafeLen(ctrSvrIP.data(), ctrSvrIP.size()));
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
        ip.fill('\0');
        str.copy(ip.data(), ip.size() - 1, 0);
        ip.back() = '\0';
    }
    std::string GetIp()
    {
        return std::string(ip.data(), SafeLen(ip.data(), ip.size()));
    }
    uint8_t mem;
    std::array<char, 16> ip;
    uint16_t port;
};
#pragma pack(pop)
#endif
