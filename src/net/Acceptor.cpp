#include "InetAddress.h"
#include "Acceptor.h"
#include "Logging.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <functional>

//创建非阻塞套接字,并设置SO_REUSEADDR和SO_REUSEPORT选项,并绑定地址
static int createNonblocking(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0){
        LOG_FATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}



Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
        acceptChannel_(loop, acceptSocket_.fd()),
        listenning_(false)
{
    LOG_DEBUG << "Acceptor::Acceptor [fd=" << acceptSocket_.fd() << "]";

    acceptSocket_.setReuseAddr(reuseport);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
};

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    //peerAddr是对端地址
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {   LOG_DEBUG << "in Acceptor::handleRead";
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "in Acceptor::handleRead";
        if(errno == EMFILE){//文件描述符用完了
            LOG_ERROR << "Acceptor::handleRead EMFILE";
            
        }
    }
}

