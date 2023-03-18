#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "TcpConnection.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Callback.h"
#include "Acceptor.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,// 不使用SO_REUSEPORT选项,默认,
                    //因为SO_REUSEPORT选项会导致多个进程绑定同一个端口时出现问题
        kReusePort,// 
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }

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

    void setThreadNum(int numThreads);
    void start();

    EventLoop *getLoop() const
    {
        return loop_;
    }

    const std::string name() const
    {
        return name_;
    }

    const std::string ipPort() const
    {
        return ipPort_;
    }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;

    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};

#endif