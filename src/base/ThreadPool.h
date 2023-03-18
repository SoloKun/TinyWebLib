#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "noncopyable.h"
#include "Thread.h"
#include "../log/Logging.h"

#include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>

class ThreadPool : noncopyable{
public:
    using ThreadFunction = std::function<void()>;
    explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
    ~ThreadPool();

    void setThreadInitCallback(const ThreadFunction& cb){
        threadInitCallback_ = cb;
    }

    void setThreadSize(int numThreads){
        threadSize_ = numThreads;
    }

    void start();

    void stop();
    const std::string& name() const{
        return name_;
    }

    size_t queueSize() const;

    void addTask(ThreadFunction& task);


private:
    bool isFull() const; //判断线程池是否满
    void runInThread(); //线程池中的线程执行的函数

    mutable std::mutex mutex_; //互斥锁 
    //mutable表示可以在const函数中修改
    std::condition_variable cond_; 
    std::string name_; //线程池名字
    ThreadFunction threadInitCallback_;//线程初始化回调函数    
    std::vector<std::unique_ptr<Thread>> threads_; //线程池中的线程
    std::deque<ThreadFunction> queue_; //任务队列

    size_t threadSize_; //任务队列最大长度
    bool running_; //线程池是否运行
};


#endif