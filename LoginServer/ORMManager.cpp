#include "ORMManager.h"
#include <cstdio>

std::unique_ptr<ORMManager> ORMManager::instance_ = nullptr;

ORMManager::ORMManager()
{
    //初始化mysql
    mysql_init(&mysql_);

    //连接数据库
    if(mysql_real_connect(&mysql_, "192.168.44.130", "root", "123456", "users", 3306, NULL, 0) == NULL)
    {
        printf("Connect database failed：%s\n", mysql_error(&mysql_));
    }
    printf("Connect database successful\n");
}

ORMManager::~ORMManager()
{
    mysql_close(&mysql_);
}

ORMManager *ORMManager::GetInstance()
{
    static std::once_flag flag;
    std::call_once(flag, [&](){
        instance_.reset(new ORMManager());
    });
    return instance_.get();
}

void ORMManager::UserRegister(const char *name, const char *acount, const char *password, const char *usercode, const char *sig_server)
{
    auto now = std::chrono::system_clock::now();
    auto nowtimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return insertClient(name, acount, password, usercode, 0, nowtimestamp, sig_server);
}

MYSQL_ROW ORMManager::UserLogin(const char *usercode)
{
    return selectClientByUsercode(usercode);
}

void ORMManager::UserDestroy(const char *usercode)
{
    return deleteClientByUsercode(usercode);
}

void ORMManager::insertClient(const char *name, const char *acount, const char *password, const char *usercode, int online, long recently_login, const char *sig_server)
{
    //新增一行
    char query[1024];
    sprintf(query, "INSERT INTO clients (USER_NAME, USER_ACOUNT, USER_PASSWD, USER_CODE, USER_ONLINE, USER_RECENTLY_LOGIN, USER_SVR_MOUNT) VALUES ('%s', '%s', '%s', '%s', '%d', '%ld', '%s')",
        name, acount, password, usercode, online, recently_login, sig_server);
    if(mysql_query(&mysql_, query))
    {
        //大于0失败
        printf("Insert failed: %s\n", mysql_error(&mysql_));
        return;
    }
    else
    {
        printf("Insert successful\n");
    }
}

void ORMManager::deleteClientByUsercode(const char *usercode)
{
    char query[1024];
    sprintf(query, "DELETE FROM clients WHERE USER_CODE = '%s'", usercode);
    if(mysql_query(&mysql_, query))
    {
        //大于0失败
        printf("Delete failed: %s\n", mysql_error(&mysql_));
        return;
    }
    else
    {
        printf("Delete successful\n");
    }
}

MYSQL_ROW ORMManager::selectClientByUsercode(const char *usercode)
{
    char query[1024];
    MYSQL_ROW row;
    MYSQL_RES* res;
    sprintf(query, "SELECT * FROM clients WHERE USER_CODE = '%s'", usercode);
    if(mysql_query(&mysql_, query))
    {
        //大于0失败
        printf("Select failed: %s\n", mysql_error(&mysql_));
        return NULL;
    }
    else
    {
        //获取查询结果
        res = mysql_store_result(&mysql_);
        if(res)
        {
            //从结果中查询行
            row = mysql_fetch_row(res);
            //释放结果
            mysql_free_result(res);
            printf("Select successful\n");
            return row;
        }
    }
    return NULL;
}
