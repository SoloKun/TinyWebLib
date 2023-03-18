#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "noncopyable.h"
#include "CurrentThread.h"
#include "TimeStamp.h"
#include "TimerQueue.h"



#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>


class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;//回调函数
    EventLoop();
    ~EventLoop();

    void loop();//循环
    void quit();//退出

    TimeStamp pollReturnTime() const { return pollReturnTime_; }//poll返回的时间

    void runInLoop(Functor cb);//在当前线程中执行回调函数

    void queueInLoop(Functor cb);//在当前线程中执行回调函数


    void wakeup();//唤醒

    void updateChannel(Channel* channel);//更新通道
    void removeChannel(Channel* channel);//移除通道
    bool hasChannel(Channel* channel);//是否有通道

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    //是否在当前线程中

    void runAt(const TimeStamp& time, Functor&& cb)//在指定时间执行回调函数
    {
        timerQueue_->addTimer(std::move(cb), time, 0.0);
    }
    void runAfter(double delay, Functor&& cb)//在指定时间后执行回调函数
    {
        TimeStamp time(addTime(TimeStamp::now(), delay));
        runAt(time, std::move(cb));
    }
    void runEvery(double interval, Functor&& cb)//每隔指定时间执行回调函数
    {
        TimeStamp time(addTime(TimeStamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), time, interval);
    }
 


private:
    void handleRead();//处理读事件
    void doPendingFunctors();//执行回调函数

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;//是否在循环
    std::atomic_bool quit_;//是否退出
    std::atomic_bool callingPendingFunctors_;//是否在执行回调函数
    const pid_t threadId_;//线程id
    TimeStamp pollReturnTime_;//poll返回的时间
    std::unique_ptr<Poller> poller_;//poller对象
    
    
    std::unique_ptr<TimerQueue> timerQueue_;//定时器队列
                    
    int wakeupFd_;//唤醒描述符
    std::unique_ptr<Channel> wakeupChannel_;//唤醒通道

    ChannelList activeChannels_;//活跃通道
    Channel* currentActiveChannel_;//当前活跃通道
    std::mutex mutex_;//互斥锁
    std::vector<Functor> pendingFunctors_;//回调函数队列
    
};



#endif