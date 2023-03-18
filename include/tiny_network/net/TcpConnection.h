#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "Callback.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "TimeStamp.h"

class Channel;
class EventLoop;
class Socket;

//std::enable_shared_from_this<TcpConnection>是一个模板类，
//它的作用是：让一个类可以安全地共享一个对象的所有权。
//这个类的一个对象可以被多个智能指针所共享，
//当最后一个智能指针销毁时，这个对象也会被销毁。
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() { return localAddr_; }
    const InetAddress &peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buf);
    void send(Buffer *message);

    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    
    void connectEstablished();
    void connectDestroyed();
    

private:
    enum StateE { 
        kConnecting, //正在连接
        kConnected,//已经连接 
        kDisconnecting, //正在断开连接
        kDisconnected //已经断开连接
    };
    void setState(StateE s) { state_ = s; }

    void handleRead(TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const std::string &message);
    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();


    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;//state_是一个原子变量，用于表示连接的状态
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;
    //highWaterMark_是一个高水位标记，
    //用于表示当输出缓冲区中的数据量大于highWaterMark_时，
    //会调用highWaterMarkCallback_回调函数

    Buffer inputBuffer_;
    Buffer outputBuffer_;
    //inputBuffer_和outputBuffer_分别是输入缓冲区和输出缓冲区

};



#endif