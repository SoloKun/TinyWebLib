#include "TimeStamp.h"

TimeStamp TimeStamp::now(){
    struct timeval tv;
    //struc timeval {
    //    time_t tv_sec; //seconds
    //    suseconds_t tv_usec; //microseconds
    //};

    gettimeofday(&tv, NULL);
    //int gettimeofday(struct timeval *tv, struct timezone *tz);
    //tv:返回的时间 tz:时区
    int64_t seconds = tv.tv_sec;
    return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}




std::string TimeStamp::toFormattedString(bool showMicroseconds) const{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(m_microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    tm *tm_time = localtime(&seconds);
    //struct tm {
    //    int tm_sec; //seconds after the minute - [0, 60]
    //    int tm_min; //minutes after the hour - [0, 59]
    //    int tm_hour; //hours since midnight - [0, 23]
    //    int tm_mday; //day of the month - [1, 31]
    //    int tm_mon; //months since January - [0, 11]
    //    int tm_year; //years since 1900
    //    int tm_wday; //days since Sunday - [0, 6]
    //    int tm_yday; //days since January 1 - [0, 365]
    //    int tm_isdst; //daylight savings time flag
    //};
    //struct tm *localtime(const time_t *timep); timep:指向time_t类型的指针,表示从1970年1月1日0时0分0秒到现在的秒数
    //返回值:指向tm结构体的指针,表示当前时间

    if(showMicroseconds){
        // 2000-01-01 00:00:00.000000
        int microseconds = static_cast<int>(m_microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
                tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec,
                microseconds);
    }
    else{
        // 2000-01-01 00:00:00
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
                tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
                tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    }
    return buf;
}


// int main(){
//     TimeStamp t = TimeStamp::now();
//     std::cout << t.toFormattedString() << std::endl;
//     std::cout << t.toFormattedString(true) << std::endl;
//     return 0;
// }