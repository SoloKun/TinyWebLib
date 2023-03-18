#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include "TimeStamp.h"
#include "Channel.h"

#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <set>

class EventLoop;
class Timer;

class TimerQueue
{
public:
    using TimerCallback = std::function<void()>;

    explicit TimerQueue(EventLoop * loop);
    ~TimerQueue();

    void addTimer(const TimerCallback &cb,
                  TimeStamp when,
                  double interval);

private:
    using Entry = std::pair<TimeStamp, Timer *>; // 以时间戳获取定时器
    //第一个参数是到期的时间戳，第二个参数是定时器。
    using TimerList = std::set<Entry>;           // 以时间戳排序,

    // 在本次循环中添加定时器
    void addTimerInLoop(Timer * timer);

    // 读事件回调函数
    void handleRead();

    // 重新设置timerfd的超时时间
    void resetTimerfd(int timerfd_, TimeStamp now);

    // 获取超时时间
    std::vector<Entry> getExpired(TimeStamp now);
    void reset(const std::vector<Entry> &expired, TimeStamp now);

    bool insert(Timer * timer); // 插入定时器

    EventLoop *loop_;        // 所属的EventLoop
    const int timerfd_;      // 定时器文件描述符
    Channel timerfdChannel_; // 定时器文件描述符对应的Channel
    TimerList timers_;       // 定时器列表

    bool callingExpiredTimers_; // 是否正在调用超时定时器
};

#endif
