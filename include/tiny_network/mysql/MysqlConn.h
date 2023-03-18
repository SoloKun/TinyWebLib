#ifndef MYSQLCONN_H
#define MYSQLCONN_H

#include <mysql/mysql.h>
#include <iostream>
#include <chrono>
#include <string>
using std::chrono::steady_clock;

class MysqlConn
{
public:
    MysqlConn();
    ~MysqlConn();
    bool connect(const std::string& user, const std::string& passwd, const std::string dbName, const std::string& ip, const unsigned int& port = 3306);
    bool update(const std::string& sql);
    bool query(const std::string& sql);
    bool next();
    std::string value(int index);
    bool transaction();
    bool commit();
    bool rollback();
    void refreshAliveTime();
    long long getAliveTime();

private:
    void freeResult();
    MYSQL *conn_ = nullptr;
    MYSQL_RES *res_ = nullptr;
    MYSQL_ROW row_ = nullptr;

    steady_clock::time_point lastActiveTime_; // 最后一次活跃时间
};

#endif