#include "EPollPoller.h"
#include <string>

const int kNew = -1;//新建的Channel
const int kAdded = 1;//已经添加到epoll中的Channel
const int kDeleted = 2;//已经从epoll中删除的Channel

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      //创建epoll文件描述符,EPOLL_CLOEXEC:设置close-on-exec标志,
      //在调用exec时关闭文件描述符,防止子进程继承该文件描述符
      //epoll_create1()函数是epoll_create()函数的增强版,它支持更多的标志
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

//这里的poll会阻塞在epoll_wait()函数上,直到有事件发生或者超时
TimeStamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    size_t numEvents = ::epoll_wait(epollfd_,
                                    &(*events_.begin()),
                                    static_cast<int>(events_.size()),
                                    timeoutMs);
    //epoll_wait(epollfd, events, maxevents, timeout);
    //epoll_wait()函数用于等待事件的产生,类似于select()调用
    //epollfd:由epoll_create()函数生成的epoll专用的文件描述符
    //events:用来从内核得到事件的集合
    //maxevents:告之内核这个events有多大,这个maxevents的值不能大于创建epoll_create()时的size
    //timeout:超时时间,单位毫秒,0会立即返回,而-1会阻塞
    //返回需要处理的事件数目,如返回0表示已超时

    int savedErrno = errno;
    TimeStamp now(TimeStamp::now());
    if(numEvents>0){
        LOG_TRACE << numEvents << " events happend";
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()){
            events_.resize(events_.size()*2);
        }
    }else if(numEvents == 0){
        LOG_TRACE << "nothing happend";
    }else{
        if(savedErrno != EINTR){
            errno = savedErrno;
            LOG_ERROR << "EPollPoller::poll()";
        }
    }
    return now;
}

void EPollPoller::updateChannel(Channel *channel){
    const int index = channel->index();
    if(index == kNew || index == kDeleted){
        //如果是新建的Channel或者已经从epoll中删除的Channel
        int fd = channel->fd();
        if(index == kNew){//如果是新建的Channel,则添加到channels_中
            channels_[fd] = channel;
        }else{
            //检查channels_中是否有该Channel
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }else{
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);//设置为已经从epoll中删除的Channel
            /*
            kDeleted状态只是用于标记该Channel对象已经被删除，
            以便于调试和错误检测，并不会影响该Channel对象的复用。
            */
        }else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const{
    
    for(int i=0; i<numEvents; ++i){
        
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        
        //events_是一个epoll_event数组,每个元素都包含一个事件的数据
        //data是一个联合体,包含一个指针和一个整数,这里用指针指向Channel
        //data.ptr指向的是Channel对象
        
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::removeChannel(Channel *channel){
    int fd = channel->fd();//获取Channel的文件描述符
    LOG_TRACE << "fd = " << fd;
    channels_.erase(fd);//从channels_中删除该Channel

    int index = channel->index();//获取该Channel的状态,是否已经添加到epoll中
    //kNew:新建的Channel,kAdded:已经添加到epoll中的Channel,kDeleted:已经从epoll中删除的Channel
    if(index == kAdded){//如果该Channel已经添加到epoll中
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
    //为什么不设置为kDeleted?
    //这是为了复用。因为 Channel 对象是在 TcpConnection 对象中被创建的，
    //并且在 TcpConnection 对象的生命周期内会被多次调用，
    //所以将 Channel 对象状态设置为 kNew 而不是 kDeleted，
    //这样 Channel 对象可以被重复使用，而不必每次都创建一个新的对象。

}

void EPollPoller::update(int operation,Channel *channel){
    epoll_event event;
    ::memset(&event, 0, sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;
    //epoll_ctl(epollfd, operation, fd, event);
    //epollfd:由epoll_create()函数生成的epoll专用的文件描述符
    //operation:表示对文件描述符的操作,有三种可能的值:
    //EPOLL_CTL_ADD:注册新的fd到epfd中
    //EPOLL_CTL_MOD:修改已经注册的fd的监听事件
    //EPOLL_CTL_DEL:从epfd中删除一个fd
    //fd:需要监听的文件描述符
    //event:告诉内核需要监听什么事
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR << "epoll_ctl op = EPOLL_CTL_DEL";
        }else{
            LOG_FATAL << "epoll_ctl op = EPOLL_CTL_ADD";
        }
    }
}