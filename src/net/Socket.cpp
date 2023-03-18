#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "Socket.h"
#include "InetAddress.h"
#include "Logging.h"

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    int ret = ::bind(sockfd_, (struct sockaddr*)localaddr.getSockAddr(), sizeof(struct sockaddr_in));
    //bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr));
    //sockfd_:套接字描述符
    //addr:地址,sockaddr_in结构体,网络字节序
    //sizeof(addr):地址长度
    //返回值:成功返回0,失败返回-1

    if (ret < 0)
    {
        LOG_FATAL << "bind error:"<<(errno);
    }
}

void Socket::listen()
{
    int ret = ::listen(sockfd_, 1024);
    //listen(fd, backlog);
    //fd:套接字描述符
    //backlog:最大连接数
    //返回值:成功返回0,失败返回-1
   
    if (ret < 0)
    {
        LOG_FATAL << "listen error";
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    ::memset (&addr, 0, addrlen);
    int connfd = ::accept4(sockfd_, (struct sockaddr*)&addr, &addrlen,
        SOCK_NONBLOCK | SOCK_CLOEXEC);
    //accept4(sockfd, addr, addrlen, flags);
    //sockfd:套接字描述符
    //addr:地址,sockaddr_in结构体,网络字节序
    //addrlen:地址长度
    //flags:标志位,SOCK_NONBLOCK:非阻塞,SOCK_CLOEXEC:子进程不继承
    //返回值:成功返回新的套接字描述符,失败返回-1
    //accept4是accept的扩展,在linux2.6.28之后才有
    
    if(connfd>=0){
        peeraddr->setSockAddr(addr);
    }else{
        LOG_ERROR<<"accept error";
    }
    return connfd;
}

void Socket::shutdownWrite()
{   //关闭写端
    //shutdown(sockfd, SHUT_WR);
    //sockfd:套接字描述符
    //SHUT_WR:关闭写端,SHUT_RD:关闭读端,SHUT_RDWR:关闭读写端
    //返回值:成功返回0,失败返回-1
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "sockets::shutdownWrite";
    }
}


//setsockopt函数用于设置套接字选项
//int setsockopt(sockfd, level, optname, optval, optlen);
//sockfd:套接字描述符
//level:选项所在的协议层.
//socket选项分为三层:套接字层,IP层,TCP层
//optname:选项名
//optval:选项值
//optlen:选项值长度
//返回值:成功返回0,失败返回-1


//不使用Nagle算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
        &optval, static_cast<socklen_t>(sizeof optval));
}

//地址复用
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
        &optval, static_cast<socklen_t>(sizeof optval));
}

//端口复用
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
        &optval, static_cast<socklen_t>(sizeof optval));
}

//keepalive
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
        &optval, static_cast<socklen_t>(sizeof optval));
}

