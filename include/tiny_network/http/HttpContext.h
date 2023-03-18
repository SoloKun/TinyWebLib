#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H


#include "HttpRequest.h"

class Buffer;
class HttpContext
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行状态
        kExpectHeaders,     // 解析请求头部状态
        kExpectBody,        // 解析请求体状态
        kGotAll,            // 解析完毕状态
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    bool parseRequest(Buffer* buf, TimeStamp receiveTime);

    bool gotAll() const
    {
        return state_ == kGotAll;
    }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const
    {
        return request_;
    }
    
private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};

#endif