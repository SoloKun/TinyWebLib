#include "Timer.h"

void Timer::restart(TimeStamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
        //重复事件，更新超时时间
    }
    else
    {
        expiration_ = TimeStamp();
        //一次性事件，超时时间置为无效值
    }
}
