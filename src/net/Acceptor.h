#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "noncopyable.h"
#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();//开始监听
private:
    void handleRead();//处理读事件

    EventLoop* loop_;//所属的EventLoop
    Socket acceptSocket_;//监听套接字
    Channel acceptChannel_;//监听套接字对应的Channel
    NewConnectionCallback newConnectionCallback_;//新连接回调函数
    bool listenning_;//是否正在监听
};




#endif