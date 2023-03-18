#ifndef ASYNCLOGGING_H_
#define ASYNCLOGGING_H_

#include "LogStream.h"//实现了日志流 主要重载了<<运算符，用于格式化日志。
#include "LogFile.h"//实现了日志文件 实现了日志文件的写入
#include "noncopyable.h"
#include "FixedBuffer.h"//实现了固定缓冲区
#include "Thread.h"//实现了线程
#include "TimeStamp.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <stdio.h>

//异步日志类，将日志的前端和后端分离，前端将日志写入缓冲区，后端将缓冲区中的日志写入文件
class AsyncLogging{
public:
    AsyncLogging(const std::string& basename, 
                 off_t rollSize, 
                 int flushInterval = 3);
    ~AsyncLogging(){
        if(running_){
            stop();
        }
    }
    //将日志写入缓冲区，由前端线程调用，前端将该函数作为out的回调函数
    //前端线程将日志写入缓冲区，后端线程将缓冲区中的日志写入文件
    void append(const char* logline, int len);
    
    void start(){
        running_ = true;
        thread_.start();
    }
    void stop(){
        //如果异步线程崩溃，会导致日志丢失，因此需要在析构函数中调用stop()函数，确保日志写入文件
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }
private:
    using Buffer = FixedBuffer<kLargeBuffer>;//一个大缓冲区
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;//缓冲区数组
    using BufferPtr = BufferVector::value_type;//value_type是vector的内部类型

    void threadFunc();//线程函数

    const int flushInterval_;//刷新间隔
    std::atomic<bool> running_;//是否运行
    std::string basename_;//日志文件名
    const off_t rollSize_;//日志文件大小
    Thread thread_;//后端线程，用于将缓冲区中的日志写入文件
    std::mutex mutex_;//互斥锁
    std::condition_variable cond_;//条件变量

    BufferPtr currentBuffer_;//当前缓冲区
    BufferPtr nextBuffer_;//空闲缓冲区
    BufferVector buffers_;//满缓冲区数组

};




#endif