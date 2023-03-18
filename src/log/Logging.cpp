#include "Logging.h"
#include "CurrentThread.h"

//用于存储线程信息,线程局部存储,每个线程都有自己的一份.
namespace ThreadInfo{
    __thread char t_errnobuf[512];//错误信息
    __thread char t_time[64];//时间
    __thread time_t t_lastSecond;//上一次的时间,用于判断是否需要更新时间
}

//获取错误信息
const char* getErrnoMsg(int savedErrno){
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf, sizeof ThreadInfo::t_errnobuf);
}

//将日志等级转换为字符串
const char* getLevelName[Logger::LogLevel::LEVEL_COUNT]
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

Logger::LogLevel initLogLevel(){
    if(::getenv("LOG_TRACE")){
        return Logger::LogLevel::TRACE;
    }else if(::getenv("LOG_DEBUG")){
        return Logger::LogLevel::DEBUG;
    }else{
        return Logger::LogLevel::INFO;
    }
}

//设置日志等级
Logger::LogLevel g_logLevel = initLogLevel();

//默认的输出函数，输出到标准输出
static void defaultOutput(const char* msg, int len){
    fwrite(msg,len,sizeof(char), stdout);
    //fwrite()函数用于将数据写入文件
    //msg:指向要被写入的元素数组的指针
    //len:每个元素的大小，以字节为单位
    //sizeof(char):要被写入的元素的个数
    //stdout:指向FILE对象的指针，该FILE对象标识了一个输出流
}

//默认的刷新函数
static void defaultFlush(){
    fflush(stdout);
    //fflush()函数用于清空指定流的缓冲区
}

//设置为默认
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int savedErrno, const char* file, int line)
    :time_(TimeStamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
   formatTime();//格式化时间
    //不用TimeStam::now().toFormattedString()
   //因为这样会多次调用now()函数，而且每次调用now()函数都会获取当前时间
    
    
    stream_ <<" Thread:"<<CurrentThread::tid()<< " ";

    stream_ <<GeneralTemplate(getLevelName[level_],6);
    //GeneralTemplate()函数用于格式化字符串

    if(savedErrno != 0){
        stream_ <<getErrnoMsg(savedErrno)<<" (errno = "<<savedErrno<<") ";
    }
}

void Logger::Impl::formatTime(){
    TimeStamp now(TimeStamp::now());
    time_t seconds =  static_cast<time_t>(now.microSecondsSinceEpoch() / TimeStamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % TimeStamp::kMicroSecondsPerSecond);

    struct tm *tm_time = localtime(&seconds);

    snprintf(ThreadInfo::t_time, sizeof ThreadInfo::t_time, 
    "%4d-%02d-%02d %02d:%02d:%02d.%06d",
            tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
            tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec,
            microseconds);
    ThreadInfo::t_lastSecond = seconds;

    char buf[32] = {0};
    snprintf(buf, sizeof buf, ".%06d ", microseconds);
    stream_ <<GeneralTemplate(ThreadInfo::t_time, 19);
}

//日志结束时调用
void Logger::Impl::finish(){
    stream_ << " - " <<GeneralTemplate(basename_.data_, basename_.size_)<<":"<<line_<<'\n';
}

//默认为INFO
Logger::Logger(const char* file, int line):
    impl_(Logger::LogLevel::INFO, 0, file, line)
{
}

Logger::Logger(const char* file, int line, LogLevel level):
    impl_(level, 0, file, line)
{
}


Logger::Logger(const char* file, int line, LogLevel level, const char* func):
    impl_(level, 0, file, line)
{
    impl_.stream_ <<func<<' ';
}


Logger::~Logger(){
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if(impl_.level_ == LogLevel::FATAL){
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level){
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out){
    g_output = out;
}

void Logger::setFlush(FlushFunc flush){
    g_flush = flush;
}

// int main(){
//     Logger::setLogLevel(Logger::LogLevel::TRACE);
//     Logger::setOutput(defaultOutput);
//     Logger::setFlush(defaultFlush);

//     LOG_TRACE<<"trace";
//     LOG_DEBUG<<"debug";
//     LOG_INFO<<"info";
//     LOG_WARN<<"warn";
//     LOG_ERROR<<"error";
//     //LOG_FATAL<<"fatal";
//     return 0;
// }