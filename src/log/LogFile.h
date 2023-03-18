#ifndef LOGGING_H_
#define LOGGING_H_

#include "FileUtil.h"

#include <mutex>
#include <memory>

//实现日志文件的写入，实现了日志文件的滚动
class LogFile{
public:
    LogFile(const std::string& basename, 
            off_t rollSize, 
            int flushInterval = 3, 
            int checkEveryN = 1024);

    ~LogFile()=default;

    void append(const char* data, int len);
    void flush();
    bool rollFile();
private:
    //获取日志文件名
    static std::string getLogFileName(const std::string& basename, time_t* now);
    //appendInLock()函数用来将数据写入文件
    void appendInLock(const char* data, int len);
    
    const std::string basename_;//日志文件名
    const off_t rollSize_;//日志文件大小
    const int flushInterval_;//刷新间隔
    const int checkEveryN_;//检查间隔,每写入checkEveryN_次，就检查一次是否需要滚动日志文件

    int count_;//计数器

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;//开始时间
    time_t lastRoll_;//上次滚动时间
    time_t lastFlush_;//上次刷新时间
    std::unique_ptr<FileUtil> file_;//文件指针
    
    const static int kRollPerSeconds_ = 60 * 60 * 24;//每天滚动一次

};



#endif