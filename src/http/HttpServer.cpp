#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"


#include <memory>

void defaultHttpCallback(const HttpRequest&,HttpResponse* resp){
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop *loop,
                    const InetAddress &listenAddr,
                    const std::string &name,
                    TcpServer::Option option)
    :server_(loop,listenAddr,name,option),
    httpCallback_(defaultHttpCallback){
        server_.setConnectionCallback(std::bind(&HttpServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&HttpServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        server_.setThreadNum(4);
    }
void HttpServer::start(){
    LOG_INFO<<"HttpServer["<<server_.name()<<"] starts listening on "<<server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn){
    if(conn->connected()){
        LOG_INFO<<"HttpServer["<<server_.name()<<"] - "<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" is UP";
    }else{
        LOG_INFO<<"HttpServer["<<server_.name()<<"] - "<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" is DOWN";
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,Buffer *buf,TimeStamp receiveTime){
    std::unique_ptr<HttpContext> context(new HttpContext);
    if(!context->parseRequest(buf,receiveTime)){
        LOG_INFO<<"HttpServer["<<server_.name()<<"] - "<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" is DOWN";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
    if(context->gotAll()){
        LOG_INFO<<"HttpServer["<<server_.name()<<"] - "<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" is DOWN";
        onRequest(conn,context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn,const HttpRequest &req){
    const std::string& connection = req.getHeader("Connection");
    bool close = connection == "close" || (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);
    httpCallback_(req,&response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if(response.closeConnection()){
        conn->shutdown();
    }
}