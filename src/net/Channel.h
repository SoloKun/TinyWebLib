#ifndef CHANNEL_H
#define CHANNEL_H

#include "noncopyable.h"
#include "TimeStamp.h"
#include "Logging.h"

#include <functional>
#include <memory>
#include <sys/epoll.h>

//前置声明
class EventLoop;
class TimeStamp;


//Channel类是对epoll_event的封装，它负责注册和注销事件，以及处理事件，
//它是一个智能指针，它的生命周期由EventLoop管理


class Channel:noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;
    //读事件回调函数,带时间戳,时间戳是事件发生的时间

    Channel(EventLoop* loop,int fd);
    ~Channel();

    void handleEvent(TimeStamp receiveTime);//处理事件

    void setReadCallback(const ReadEventCallback& cb)
    {readCallback_ = cb;}
    void setWriteCallback(const EventCallback& cb)
    {writeCallback_ = cb;}
    void setCloseCallback(const EventCallback& cb)
    {closeCallback_ = cb;}
    void setErrorCallback(const EventCallback& cb)
    {errorCallback_ = cb;}

    void tie(const std::shared_ptr<void>&);//tie_指向一个对象，该对象的生命周期由tie_管理

    int fd() const {return fd_;}//返回文件描述符
    int events() const {return events_;}//返回关注的事件
    void set_revents(int revt) {revents_ = revt;}//设置返回的事件

    void enableReading() {events_ |= kReadEvent; update();}//关注读事件
    void disableReading() {events_ &= ~kReadEvent; update();}//取消关注读事件
    void enableWriting() {events_ |= kWriteEvent; update();}//关注写事件
    void disableWriting() {events_ &= ~kWriteEvent; update();}//取消关注写事件
    void disableAll() {events_ = kNoneEvent; update();}//取消关注所有事件

    bool isNoneEvent() const { return events_ == kNoneEvent; }//是否是空闲事件
    bool isWriting() const { return events_ & kWriteEvent; }//是否是写事件
    bool isReading() const { return events_ & kReadEvent; }//是否是读事件

    int index() { return index_; }//返回在Poller中的索引
    void set_index(int idx) { index_ = idx; }//设置在Poller中的索引

    EventLoop* ownerLoop() { return loop_; }//返回所属的EventLoop
    void remove();//移除Channel

private:
    void update();
    void handleEventWithGuard(TimeStamp receiveTime);//处理事件,带锁

    static const int kNoneEvent;//空闲事件
    static const int kReadEvent;//读事件
    static const int kWriteEvent;//写事件

    EventLoop* loop_;//所属的EventLoop
    const int fd_;//文件描述符
    int events_;//关注的事件
    int revents_;//返回的事件
    int index_;//在Poller中的索引

    std::weak_ptr<void> tie_;//tie_是一个弱引用，它指向一个对象，该对象的生命周期由tie_管理
    bool tied_;//tie_是否有效

    ReadEventCallback readCallback_;//读事件回调函数
    EventCallback writeCallback_;//写事件回调函数
    EventCallback closeCallback_;//关闭事件回调函数
    EventCallback errorCallback_;//错误事件回调函数
};


#endif