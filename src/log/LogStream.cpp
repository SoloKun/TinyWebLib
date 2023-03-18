#include "LogStream.h"

static const char digits[] = {'9', '8', '7', '6', '5', '4', '3', '2', '1',
                              '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

template <typename T>
// 转换整数
void LogStream::formatInteger(T v)
{
    // 这里的buffer_是一个FixedBuffer<kSmallBuffer>类型的对象
    // 如果缓冲区的剩余空间大于数字的最大长度
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *start = buffer_.current(); // 将当前指针指向缓冲区的开头
        char *cur = start;
        const char *zero = digits + 9; // zero指向字符'0'
        bool negative = v < 0;         // 判断v是否为负数
        if (negative)
        {
            v = -v;
        }
        while (v > 0)
        {
            int digit = v % 10;
            v /= 10;
            *cur++ = zero[digit];
        }
        if (negative)
        {
            *cur++ = '-';
        }
        *cur = '\0';
        std::reverse(start, cur);
        // 将当前指针向后移动cur - start个字节
        buffer_.add(static_cast<int>(cur - start));
    }
}

//都转换成int类型，然后调用formatInteger函数处理

LogStream &LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v){
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v){
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v){
    formatInteger(v);
    return *this;
}
LogStream &LogStream::operator<<(long v){
    formatInteger(v);
    return *this;
}
LogStream &LogStream::operator<<(unsigned long v){
    formatInteger(v);
    return *this;
}
LogStream &LogStream::operator<<(long long v){
    formatInteger(v);
    return *this;
}
LogStream &LogStream::operator<<(unsigned long long v){
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(float v){
    *this << static_cast<double>(v);
    return *this;
}


LogStream &LogStream::operator<<(double v){
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        //%.12g表示以十进制形式输出，小数点后保留12位有效数字
        buffer_.add(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(char c){
    buffer_.append(&c, 1);
    return *this;
}

LogStream &LogStream::operator<<(const char *str){
    if (str)
    {
        buffer_.append(str, strlen(str));
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str){
    return operator<<(reinterpret_cast<const char *>(str));
}

LogStream &LogStream::operator<<(const std::string &str){
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream &LogStream::operator<<(const Buffer &buf){
    *this << buf.toString();
    return *this;
}

LogStream &LogStream::operator<<(const void *data){
    *this << static_cast<const char *>(data);
    return *this;
}

LogStream &LogStream::operator<<(const GeneralTemplate &g){
    buffer_.append(g.data_, g.length_);
    return *this;
}

// #include <iostream>

// int main(){
//     LogStream log;
//     double d = 3.1415926;
//     log << "hello world" << 123 << 3.14 << "hello world" << -123 << -3.14<< d;
//     std::cout << log.buffer().toString() << std::endl;
//     return 0;
// }