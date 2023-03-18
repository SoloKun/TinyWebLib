#include "LogFile.h"

LogFile::LogFile(const std::string& basename, off_t rollSize, int flushInterval, int checkEveryN)
    :basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(new std::mutex),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0){
        rollFile();
    }

void LogFile::append(const char* data, int len){
    std::lock_guard<std::mutex> lock(*mutex_);
    appendInLock(data, len);
}

void LogFile::appendInLock(const char* data, int len){
    file_->append(data, len);
    //append()函数用来将数据写入文件
    //可写的数据大于日志文件大小时，滚动日志文件
    if(file_->writtenBytes() > rollSize_){
        rollFile();
    }else{
        //可写的数据小于日志文件大小时，判断是否需要刷新或滚动
        ++count_;
        if(count_ >= checkEveryN_){
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if(thisPeriod_ != startOfPeriod_){//本次时间与开始时间不同，滚动日志文件
                rollFile();
            }else if(now - lastFlush_ > flushInterval_){//本次时间与上次刷新时间间隔大于刷新间隔，刷新日志文件
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

void LogFile::flush(){
    std::lock_guard<std::mutex> lock(*mutex_);
    //lock_guard是一个RAII封装，构造时加锁，析构时解锁
    file_->flush();
}

bool LogFile::rollFile(){
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    //计算当前时间与开始时间的间隔，如果间隔大于一天，则滚动日志文件
    if(now > lastRoll_){
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new FileUtil(filename));
        return true;
    }
    return false;
}


std::string LogFile::getLogFileName(const std::string& basename, time_t* now){
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;
    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    //localtime_r()函数将time_t类型的时间转换为tm类型的时间
    strftime(timebuf, sizeof(timebuf), ".%Y-%m-%d %H:%M:%S", &tm);
    //strftime()函数将tm类型的时间转换为字符串
    filename += timebuf;
    filename += ".log";
    return filename;
}

// int main(){
//     LogFile logFile("test", 1024, 3, 1024);
//     logFile.append("hello world", 11);
//     logFile.flush();

//     return 0;
// }