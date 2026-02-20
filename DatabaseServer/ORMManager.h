#ifndef _ORMMANAGER_H_
#define _ORMMANAGER_H_
#include <memory>
#include <mutex>
#include <mysql/mysql.h>

class ORMManager
{
public:
    ~ORMManager();
    static ORMManager* GetInstance();  //全局实例
    void UserRegister(const char* name, const char* acount, const char* password, const char* usercode,const char* sig_server);
    MYSQL_ROW UserLogin(const char* name, const char* acount, const char* password, const char* usercode);
    void UserDestroy(const char* usercode);
protected:
    void insertClient(const char* name, const char* acount, const char* password, const char* usercode, int online, long recently_login, const char* sig_server);
     void deleteClientByUsercode(const char* usercode);
    MYSQL_ROW selectClientByUsercode(const char* usercode);
private:
    ORMManager();
    MYSQL mysql_;
private:
    static std::unique_ptr<ORMManager> instance_;
};
#endif
