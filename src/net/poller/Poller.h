#ifndef POLLER_H
#define POLLER_H

#include "noncopyable.h"
#include "TimeStamp.h"
#include "Channel.h"


#include <unordered_map>
#include <vector>

//库中多路时间分发器的核心IO复用类
class Poller:noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;//Channel列表
    Poller(EventLoop* loop);
    virtual ~Poller()=default;
    //虚析构函数,基类指针指向派生类对象时，
    //调用delete时，会调用派生类的析构函数,而不是基类的析构函数
    //如果基类的析构函数不是虚函数，那么delete时，
    //只会调用基类的析构函数 产生内存泄漏

    virtual TimeStamp poll(int timeoutMs,ChannelList* activeChannels)=0;
    //纯虚函数,返回活跃的Channel列表
    //timeoutMs:超时时间
    //activeChannels:活跃的Channel列表

    virtual void updateChannel(Channel* channel)=0;
    //纯虚函数,更新Channel
    //channel:需要更新的Channel

    virtual void removeChannel(Channel* channel)=0;
    //纯虚函数,移除Channel
    //channel:需要移除的Channel

    bool hasChannel(Channel* channel) const;
    //判断是否有该Channel

    static Poller* newDefaultPoller(EventLoop* loop);
    //创建Poller对象

protected:
    using ChannelMap = std::unordered_map<int,Channel*>;//Channel映射表

    ChannelMap channels_;//Channel映射表

private:
    EventLoop* ownerLoop_;//拥有该Poller的EventLoop
};


#endif