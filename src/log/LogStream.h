#ifndef LOGSTREAM_H_
#define LOGSTREAM_H_
#include "FixedBuffer.h"
#include "noncopyable.h"

#include <string>
#include <algorithm>



//模版类，用于格式化日志
class GeneralTemplate: noncopyable {
public:
    const char*data_;
    int length_;

    GeneralTemplate():data_(NULL), length_(0) {}

    explicit GeneralTemplate(const char *str,int len):data_(str), length_(len) {}


};

//实现日志的前端，主要是重载了<<运算符，用于格式化日志
class LogStream: noncopyable {
public:
    using Buffer = FixedBuffer<kSmallBuffer>;//缓冲区的类型

    void append(const char*data, int len) { buffer_.append(data, len); }//将data指向的字符串添加到缓冲区中
    const Buffer& buffer() const { return buffer_; }//返回缓冲区
    void resetBuffer() { buffer_.reset(); }//将缓冲区清空

    
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float);
    LogStream& operator<<(double);

    LogStream& operator<<(char);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);
    LogStream& operator<<(const Buffer&);
    LogStream& operator<<(const void*);

    LogStream& operator<<(const GeneralTemplate&);

private:
    static const int kMaxNumericSize = 48;//数字的最大长度
    template<typename T>
    void formatInteger(T);//格式化整数
    Buffer buffer_;//缓冲区
};

#endif
