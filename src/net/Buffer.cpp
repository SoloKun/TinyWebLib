#include "Buffer.h"


//\n 是Windows下的换行符，\r\n是Linux下的换行符
const char Buffer::kCRLF[] = "\r\n";


void Buffer::retrieveAll(){
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

void Buffer::retrieve(size_t len){
    assert(len <= readableBytes());
    if(len < readableBytes()){
        readerIndex_ += len;
    }else{
        retrieveAll();
    }
}


std::string Buffer::retrieveAsString(size_t len){
    assert(len <= readableBytes());
    std::string result(beginRead(), len);
    retrieve(len);
    return result;
}

std::string Buffer::retrieveAllAsString(){
    return retrieveAsString(readableBytes());
}

void Buffer::retrieveUntil(const char* end){
    assert(beginRead() <= end);
    assert(end <= beginWrite());
    retrieve(end - beginRead());
}

void Buffer::ensureWritableBytes(size_t len){
    if(writableBytes() < len){
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

void Buffer::append(const std::string& str){
    append(str.data(), str.length());
}

void Buffer::append(const char* data, size_t len){
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

const char* Buffer::findCRLF() const{
    const char* crlf = std::search(beginRead(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
}

ssize_t Buffer::readFd(int fd, int* savedErrno){
    char extrabuf[65536] = {0};//二级缓冲区
    struct iovec vec[2];
    //struct iovec {
    //    void  *iov_base;    /*首地址*/
    //    size_t iov_len;     /*长度*/
    //};
    const size_t writable = writableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    //buffer_小于64k，使用二级缓冲区，否则直接使用buffer_
    const ssize_t n = ::readv(fd, vec, iovcnt);
    //readv优先将数据读到buffer_中，如果buffer_不够，再读到extrabuf中
    //n < 0 读取失败, n == 0 读取到文件末尾, n > 0 读取到数据
    
    if(n < 0){
        *savedErrno = errno;
    }else if(static_cast<size_t>(n) <= writable){
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}


ssize_t Buffer::writeFd(int fd, int* savedErrno){
    size_t n = readableBytes();
    ssize_t nwrite = ::write(fd, beginRead(), n);
    if(nwrite < 0){
        *savedErrno = errno;
    }else{
        retrieve(nwrite);
    }
    return nwrite;
}


void Buffer::makeSpace(size_t len){
    //如果buffer_的可写空间不够，且可读空间加上可写空间不够len，那么就resize
    if(writableBytes() + prependableBytes() < len + kCheapPrepend){
        buffer_.resize(writerIndex_ + len);
    }else{
        assert(kCheapPrepend < readerIndex_);
        //将可读数据移到buffer_的前面
        size_t readable = readableBytes();
        std::copy(begin()+readerIndex_, begin()+writerIndex_, begin()+kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        assert(readable == readableBytes());
    }
}
