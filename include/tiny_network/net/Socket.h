#ifndef SOCKET_H
#define SOCKET_H

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:

    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    ~Socket();

    int fd() const { return sockfd_; }

    void bindAddress(const InetAddress &localaddr);

    void listen();

    int accept(InetAddress *peeraddr);

    void shutdownWrite();//关闭写端

    void setTcpNoDelay(bool on);//设置TCP_NODELAY
    void setReuseAddr(bool on);//设置地址复用
    void setReusePort(bool on);//设置端口复用
    void setKeepAlive(bool on);//设置keepalive

private:
    const int sockfd_;

};


#endif