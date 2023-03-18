#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "TcpServer.h"
#include "noncopyable.h"
#include "Logging.h"
#include <string>

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable{
public:
    using HttpCallback = std::function<void(const HttpRequest&,HttpResponse*)>;

    HttpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string& name,
                TcpServer::Option option = TcpServer::kNoReusePort);

    EventLoop* getLoop() const{
        return server_.getLoop();
    }

    void setHttpCallback(const HttpCallback& cb){
        httpCallback_ = cb;
    }
    void start();
private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,TimeStamp receiveTime);
    void onRequest(const TcpConnectionPtr& conn,const HttpRequest& req);
    TcpServer server_;
    HttpCallback httpCallback_;
};



#endif