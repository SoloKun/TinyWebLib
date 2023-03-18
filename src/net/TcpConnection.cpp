#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logging.h"

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "TcpConnection::TcpConnection() - loop is null";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64MB
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << state_;
}

void TcpConnection::send(const std::string &message)
{
    if (state_ == kConnected)
    { // 如果连接处于已连接状态，就调用sendInLoop()函数发送数据
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else // 如果不是在IO线程中，就将数据发送任务添加到IO线程的任务队列中
        {
            void (TcpConnection::*fp)(const std::string &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, message));
        }
    }
}

void TcpConnection::send(Buffer *message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message->beginWrite(), message->readableBytes());
            message->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const std::string &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, message->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const std::string &message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // 如果当前连接处于可写状态，就直接调用write()函数发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {//channel_->isWriting()为false，说明当前连接处于可写状态
    //如果channel_->isWriting()为true，说明当前连接处于不可写状态。表明上一次发送的数据没有发送完毕
    //outputBuffer_.readableBytes()为0，说明发送缓冲区为空,可以直接发送数据。
    //如果发送缓冲区不为空，说明上一次发送的数据没有发送完毕，需要等待下一次可写事件再发送
    
        nwrote = ::write(channel_->fd(), message, len);
        //这里channel_->fd()返回的是socket的文件描述符
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            //如果发送的数据长度小于要发送的数据长度，
            //说明发送缓冲区已满，剩余的数据需要等待下一次可写事件再发送
            if (remaining == 0 && writeCompleteCallback_)
            {//如果发送的数据长度等于要发送的数据长度，说明发送缓冲区已经全部发送完毕
                //此时调用writeCompleteCallback_()回调函数
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                //shared_from_this()返回一个shared_ptr对象，指向当前对象，这里是TcpConnection对象
            }
        }
        else
        {
            nwrote = 0;
            //如果发送数据出错，且错误码是EWOULDBLOCK，说明发送缓冲区已满，剩余的数据需要等待下一次可写事件再发送
            if (errno != EWOULDBLOCK)//如果errno不是EWOULDBLOCK，说明发送数据出错
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();//发送缓冲区中已有的数据长度
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {//如果发送缓冲区中已有的数据长度加上要发送的数据长度大于highWaterMark_，说明发送缓冲区已满
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(message) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();//注册可写事件
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{

    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{

    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(TimeStamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = ::write(channel_->fd(),
                            outputBuffer_.beginWrite(), outputBuffer_.readableBytes());
        //这里是写入socket的文件描述符
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }else{
        LOG_TRACE << "Connection fd = " << channel_->fd() << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
   
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int optval;
    int err = 0;
    socklen_t optlen = sizeof optval;
    //getsockopt()函数用于获取与某个套接字关联的选项的当前值
     if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)){
        err = errno;
     }else{
        err = optval;
     }
    LOG_ERROR << "TcpConnection::handleError [" << name_.c_str()
              << "] - SO_ERROR = " << err;

}


