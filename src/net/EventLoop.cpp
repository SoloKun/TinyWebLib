#include "EventLoop.h"
#include "Logging.h"
#include "Poller.h"
__thread EventLoop *t_loopInThisThread = nullptr;
//定义了一个指向 EventLoop 对象的线程本地指针 
//t_loopInThisThread，并将其初始化为 nullptr。
//该指针的作用是跟踪当前线程是否正在运行 EventLoop 对象，
//以及获取当前线程的 EventLoop 对象的指针。
//这里使用了 C++11 引入的线程局部存储（thread-local storage）特性，
//__thread 关键字表示该变量是线程局部变量，每个线程都有一份独立的拷贝。
//在多线程环境下，通过该指针可以避免多个线程同时访问同一个 EventLoop 对象的问题。


const int kPollTimeMs = 10000;//poll的超时时间

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    //EFD_NONBLOCK：设置为非阻塞模式,可以在没有数据可读时立即返回,
    //EFD_CLOEXEC：设置为close-on-exec模式,
    //即在执行exec函数族时关闭该文件描述符,防止子进程继承该文件描述符
    if (evtfd < 0)
    {
        LOG_FATAL << "Failed in eventfd";
        abort();
    }
    return evtfd;
}
/*
createEventfd()是一个封装了Linux系统调用的函数，
用于创建一个eventfd对象。eventfd是一个非常简单的事件通知机制，
主要用于线程间通信，它在内核中创建一个可读可写的文件描述符，
并且可以通过在该文件描述符上进行读写操作来进行线程间通信。
当文件描述符上的数据可读时，
可以通过select、poll、epoll等I/O多路复用机制监视该文件描述符
并进行相应的处理。在本代码中，eventfd主要用于唤醒事件循环，
以便及时处理新的事件。
*/

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      //这里的poller_充当主要的IO复用类，它是一个多路事件分发器的核心IO复用类
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    LOG_DEBUG << "EventLoop created wakeupFd " << wakeupChannel_->fd();
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    // 设置wakeupChannel_为可读事件，这样当有事件发生时，wakeupChannel_会被添加到poller_中
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO << "EventLoop " << this << " start looping";
    while(!quit_){
        activeChannels_.clear();//清空activeChannels_,防止上次的数据影响本次的数据
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        //这里会阻塞kPollTimeMs时间,直到有活跃的channel或者超时
        //传入pooler_的参数为kPollTimeMs和activeChannels_
        //kPollTimeMs为poll的超时时间，activeChannels_为活跃的channel
        //poller_->poll()返回的是活跃的channel的数量
        for (Channel *channel : activeChannels_)//遍历活跃的channel
        {
            channel->handleEvent(pollReturnTime_);
        }
        
        doPendingFunctors();
        //执行pendingFunctors_中的函数,这里的函数是在其他线程中添加的
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{//如果当前线程是loop线程,则直接执行cb,否则将cb添加到pendingFunctors_中
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

//添加回调函数到pendingFunctors_中
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if(!isInLoopThread() || callingPendingFunctors_)
    {//如果当前线程不是loop线程,或者正在执行pendingFunctors_中的函数,则唤醒loop()
        wakeup();
    }
}

//供其它线程调用,唤醒loop()
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    //向wakeupFd_写入数据,这样wakeupChannel_就会被添加到poller_中,从而唤醒loop()
    //这里唤醒的是创建EventLoop对象的线程
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//将pendingFunctors_中的函数赋值给functors
    }

    for (const Functor &functor : functors)//遍历functors,执行其中的函数
    {
        functor();
    }
    callingPendingFunctors_ = false;
}


// int main()
// {
//     EventLoop loop;
//     loop.loop();
//     //测试
//     loop.runInLoop([](){LOG_INFO<<"runInLoop";});
//     loop.queueInLoop([](){LOG_INFO<<"queueInLoop";});


//     return 0;
// }