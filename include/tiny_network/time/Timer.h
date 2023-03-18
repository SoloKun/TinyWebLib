#ifndef TIMER_H
#define TIMER_H


#include "noncopyable.h"
#include "TimeStamp.h"

#include <functional>

//定时器类，用于定时执行回调函数，定时器的超时时间是绝对时间
class Timer : noncopyable
{
public:
    using TimerCallback = std::function<void()>;

    Timer(const TimerCallback& cb, TimeStamp when, double interval)
        : callback_(move(cb)),//move 语义转移,避免拷贝,提高效率
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0)
    { }

    void run() const
    {
        callback_();
    }
    TimeStamp expiration() const { return expiration_; }

    bool repeat() const { return repeat_; }


    void restart(TimeStamp now);

private:

    const TimerCallback callback_; //回调函数
    TimeStamp expiration_; //超时时间
    const double interval_; //超时时间间隔,一次性事件为0
    const bool repeat_; //是否重复，一次性事件为false
};



#endif