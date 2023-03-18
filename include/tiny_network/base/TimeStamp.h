#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <sys/time.h>
#include <string>
#include <iostream>
class TimeStamp
{
public:
    TimeStamp():m_microSecondsSinceEpoch_(0){}

    explicit TimeStamp(int64_t microSecondsSinceEpochArg)
        :m_microSecondsSinceEpoch_(microSecondsSinceEpochArg){}

    static TimeStamp now();//返回当前时间戳

    std::string toString() const;

    std::string toFormattedString(bool showMicroseconds = false) const;

    //返回时间戳的微秒数
    int64_t microSecondsSinceEpoch() const { return m_microSecondsSinceEpoch_; }

    //返回时间戳的秒数
    time_t secondsSinceEpoch() const
    { return static_cast<time_t>(m_microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    //失效时间戳，返回为0的时间戳
    static TimeStamp invalid()
    {
        return TimeStamp();
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000; //1s = 1000ms = 1000 * 1000us

private:
    int64_t m_microSecondsSinceEpoch_;//时间戳,以微秒为单位,表示从1970-1-1开始到现在的微秒数
};

//比较两个时间戳的大小
inline bool operator<(TimeStamp lhs, TimeStamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

//给时间戳加上秒数，返回新的时间戳
inline TimeStamp addTime(TimeStamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
    return TimeStamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif