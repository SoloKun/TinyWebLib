#ifndef THREAD_H
#define THREAD_H

#include "noncopyable.h"
#include "CurrentThread.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <semaphore.h>


class Thread : noncopyable{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    void start();//启动线程
    void join();//加入线程

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }//获取线程数


private:
    void setDefaultName();//设置默认线程名

    bool started_;//线程是否启动
    bool joined_;//连接状态 线程为连接状态时，线程结束后会自动释放资源。
    std::shared_ptr<std::thread> thread_;//线程指针
    //shared_ptr是一个智能指针，它的行为类似于指针，但它同时拥有多个所有者。
    //当最后一个所有者被销毁时，它所指向的对象也会被销毁。
    
    pid_t tid_;//线程id

    ThreadFunc func_;//线程函数
    std::string name_;//线程名
    static std::atomic_int32_t numCreated_;//线程索引

};

#endif