#ifndef LOGGING_H
#define LOGGING_H



#include "TimeStamp.h"
#include "LogStream.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <functional>
#include <sys/time.h>

//用于提取文件名
class SourceFile {
public:
    explicit SourceFile(const char *filename):data_(filename){
        const char *slash = strrchr(filename, '/');
        //2022-1-1/test.log
        //strrchr()函数用来找出字符串中最后一次出现的指定字符的位置
        if (slash){
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }


    const char *data_;
    int size_;
};

//具体实现日志的输出，日志的输出函数，日志的刷新函数，已经定义了不同日志的宏
class Logger{
public:
    enum LogLevel{
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT,//用于计数
    };

    Logger(const char* file, int line);
    Logger(const char* file, int line, LogLevel level);
    Logger(const char* file, int line, LogLevel level, const char* func);
    ~Logger();

    //输出流
    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    //设置输出函数,刷新函数
    using OutputFunc = std::function<void(const char* msg, int len)>;
    using FlushFunc = std::function<void()>;

    
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);


private:
    //具体的日志实现类
    class Impl{
    public:
        using LogLevel = Logger::LogLevel;
        //构造函数,初始化日志等级,错误码,文件名,行号
        Impl(LogLevel level, int savedErrno, const char*file, int line);
        
        void formatTime();
        void finish();

        TimeStamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };
    Impl impl_;
};

//日志宏定义
extern Logger::LogLevel g_logLevel;//日志等级

inline Logger::LogLevel Logger::logLevel(){
    return g_logLevel;
}

const char* getErrnoMsg(int savedErrno);
//__func__是C++11中的关键字，用于获取当前函数的函数名
//__FILE__是预定义宏，用于获取当前文件名
//__LINE__是预定义宏，用于获取当前行号
#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) \
    Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
    Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) \
    Logger(__FILE__, __LINE__).stream()

    
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()



#endif