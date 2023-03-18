#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>
#include <atomic>   
#include <sys/uio.h>
#include <unistd.h>

class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    //预留8个字节，用于存放一些辅助信息
    static const size_t kInitialSize = 1024;
    //初始化时的缓冲区大小
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),//buffer_的大小为kCheapPrepend+initialSize
          readerIndex_(kCheapPrepend),//读指针的初始位置为kCheapPrepend
          writerIndex_(kCheapPrepend)//写指针的初始位置为kCheapPrepend
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const { return readerIndex_;}


    //实现两个版本，一个是const，一个是非const，这样可以在const对象上调用，也可以在非const对象上调用。
    //返回可读数据的首地址
    //char *beginRead() { return begin() + readerIndex_; }

    const char* beginRead() const { return begin() + readerIndex_; }
    
    //返回可写数据的首地址
    char* beginWrite() { return begin() + writerIndex_; }

    const char* beginWrite() const { return begin() + writerIndex_; } 

    //将读指针后移len个字节
    void retrieve(size_t len);

    //将指针移动到开始处
    void retrieveAll();

    //Debug 将缓冲区中的数据全部打印出来
    std::string GetBufferAllAsString() const{
        return std::string(beginRead(), readableBytes());
    }
    
    std::string retrieveAsString(size_t len);

    std::string retrieveAllAsString();

    //将读指针移动到end处,主要用于查找\r\n
    void retrieveUntil(const char* end);

    //确认可写空间是否足够，如果不够，就扩容
    void ensureWritableBytes(size_t len);

    //将数据(str)写入缓冲区
    void append(const std::string& str);

    //写data数据，长度为len
    void append(const char* data, size_t len);

    //寻找\r\n，返回\r\n的地址，用于查找一行数据
    const char* findCRLF() const;

    ssize_t readFd(int fd, int* savedErrno);

    ssize_t writeFd(int fd, int* savedErrno);

private:
    void makeSpace(size_t len);
    char* begin() { return &*buffer_.begin(); }
    //begin是迭代器，先解引用，再取地址才是指针
    const char* begin() const { return &*buffer_.begin(); }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    static const char kCRLF[];//用于表示换行符
};



#endif