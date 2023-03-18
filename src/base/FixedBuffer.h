#ifndef FIXED_BUFFER_H_
#define FIXED_BUFFER_H_

//这个类用来存储日志信息，它的大小是固定的，不会随着日志信息的增多而增大

#include <assert.h>
#include <string.h> 
#include <strings.h>
#include <string>

#include "noncopyable.h"

const int kSmallBuffer = 4000;//小缓冲区的大小
const int kLargeBuffer = 4000 * 1000;//大缓冲区的大小

template<int SIZE>
class FixedBuffer : noncopyable {
public:
    //构造函数，初始化缓冲区，将当前指针指向缓冲区的开头
    FixedBuffer():cur_(data_){};

    void append(const char*buf, size_t len) {
        if(static_cast<size_t>(avail()) > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }//返回缓冲区的开头

    int length() const { return static_cast<int>(cur_ - data_); }//返回缓冲区的长度

    char* current() { return cur_; }//返回当前指针

    int avail() const { return static_cast<int>(end() - cur_); }//返回缓冲区剩余的空间

    void add(size_t len) { cur_ += len; }//将当前指针向后移动len个字节

    void reset() { cur_ = data_; }//将当前指针指向缓冲区的开头

    void bzero() { ::bzero(data_, sizeof data_); }//将缓冲区清零

    //将缓冲区的内容转换成string类型
    std::string toString() const { return std::string(data_, length()); }


private:
    
    const char*end() const { return data_ + sizeof data_; }//缓冲区的末尾

    char data_[SIZE];//缓冲区
    char* cur_;//当前指针


};

#endif