#include "ConnectionPool.h"
#include "Logging.h"


#include <fstream>
#include <thread>
#include <assert.h>

ConnectionPool* ConnectionPool::getInstance()
{
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::ConnectionPool(){
    //读取配置文件
    std::ifstream ifs("/home/kkk/桌面/webmuduo/mysql/test/mysql.conf");
    //格式: ip user passwd dbName port minSize maxSize timeout maxIdleTime
    assert(ifs.is_open());
    ifs>>ip_>>user_>>passwd_>>dbName_>>port_>>minSize_>>maxSize_>>timeout_>>maxIdleTime_;
    ifs.close();
    //初始化连接池
    for(int i = 0; i < minSize_; i++){
        addConnection();
        currentSize_++;
    }
    std::thread producer(&ConnectionPool::produceConnection, this);
    std::thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}

ConnectionPool::~ConnectionPool(){
    std::lock_guard<std::mutex> lock(mtx_);
    while(!connQueue_.empty()){
        MysqlConn *conn = connQueue_.front();
        connQueue_.pop();
        currentSize_--;
        delete conn;
    }
}

void ConnectionPool::produceConnection(){
    while(true){
        std::unique_lock<std::mutex> lock(mtx_);
        while(!connQueue_.empty()){
            cond_.wait(lock);
        }
        if(currentSize_ < maxSize_){
            addConnection();
            currentSize_++;
            cond_.notify_all();
        }
    }
}

void ConnectionPool::recycleConnection(){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(500));
        std::lock_guard<std::mutex> lock(mtx_);
        while(connQueue_.size() > minSize_){
            MysqlConn *conn = connQueue_.front();
            if(conn->getAliveTime()>=maxIdleTime_){
                connQueue_.pop();
                currentSize_--;
                delete conn;
            }
            else{
                break;
            }
        }
    }
}

void ConnectionPool::addConnection(){
    MysqlConn *conn = new MysqlConn();
    conn->connect(user_, passwd_, dbName_, ip_, port_);
    conn->refreshAliveTime();
    connQueue_.push(conn);
    currentSize_++;
}

std::shared_ptr<MysqlConn> ConnectionPool::getConnection(){
    std::unique_lock<std::mutex> lock(mtx_);
    if (connQueue_.empty())
    {
        while(connQueue_.empty()){
            if(std::cv_status::timeout == cond_.wait_for(lock, std::chrono::seconds(timeout_))){
                LOG_ERROR<<"get connection timeout";
                if(connQueue_.empty()){
                    continue;
                }
            }
        }
    }
    std::shared_ptr<MysqlConn> connptr(connQueue_.front(), 
    [this](MysqlConn *conn){
        std::lock_guard<std::mutex> lock(mtx_);
        conn->refreshAliveTime();
        connQueue_.push(conn);
    });
    connQueue_.pop();
    cond_.notify_all();
    return connptr;
}