#ifndef EPOLL_POLLER_H
#define EPOLL_POLLER_H

#include "Poller.h"
#include "Logging.h"
#include "TimeStamp.h"

#include <vector>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

//EPollPoller类是Poller类的派生类，它是一个多路事件分发器的核心IO复用类
class EPollPoller:public Poller
{
    using EventList = std::vector<epoll_event>;//epoll_event列表
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    TimeStamp poll(int timeoutMs,ChannelList* activeChannels) override;
    //返回活跃的Channel列表
    //timeoutMs:超时时间
    //activeChannels:活跃的Channel列表

    void updateChannel(Channel* channel) override;
    //更新Channel
    //channel:需要更新的Channel

    void removeChannel(Channel* channel) override;
    //移除Channel
    //channel:需要移除的Channel


private:
    static const int kInitEventListSize = 16;//初始化epoll_event列表的大小

    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;

    void update(int operation,Channel* channel);

    int epollfd_;//epoll文件描述符

    EventList events_;//epoll_event列表
};






#endif