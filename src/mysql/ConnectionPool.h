#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include "MysqlConn.h"

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class ConnectionPool
{
public:
    static ConnectionPool *getInstance();
    ~ConnectionPool();
    std::shared_ptr<MysqlConn> getConnection();

private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    void produceConnection();
    void recycleConnection();
    void addConnection();

    std::string ip_;
    std::string user_;
    std::string passwd_;
    std::string dbName_;
    unsigned short port_;
    int minSize_;
    int maxSize_;
    int currentSize_;
    int timeout_;
    int maxIdleTime_;
    std::queue<MysqlConn *> connQueue_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

#endif