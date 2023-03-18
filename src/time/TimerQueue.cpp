#include "TimerQueue.h"
#include "Timer.h"
#include "EventLoop.h"
#include "Logging.h"
#include "Channel.h"


// 创建定时器文件描述符，返回文件描述符
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    //CLOCK_MONOTONIC:系统启动到现在的时间
    //TFD_NONBLOCK:非阻塞
    //TFD_CLOEXEC:子进程不继承

    if (timerfd < 0)
    {
        LOG_ERROR << "Failed in timerfd_create";
    }
    return timerfd;
}
//验证
void ReadTimerfd(int timerfd){
    uint64_t read_byte;
    ssize_t n = ::read(timerfd, &read_byte, sizeof read_byte);
    /*
    当使用 read 函数从一个 timerfd 文件描述符读取数据时，
    它会返回一个8字节的数据块，其中包含了两个字段：
    uint64_t 类型的 expirations 字段表示已经到期的定时器事件数量，
    即在上一次 read 调用之后已经到期的事件数量。
    uint64_t 类型的 now 字段表示当前的时间戳，即时钟的当前时间。
    */
    if (n != sizeof read_byte)
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

//初始化绑定loop_和timerfd_
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_()
{
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    //设置读事件回调函数
    
    timerfdChannel_.enableReading();
    //注册读事件,并且设置为可读
}


TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (const Entry& timer : timers_)
    {
        delete timer.second;
    }
}


void TimerQueue::addTimer(const TimerCallback& cb,
                            TimeStamp when,
                            double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    //将定时器添加到loop_中,并且在loop_中执行,一直循环
}


void TimerQueue::addTimerInLoop(Timer* timer)
{
    bool earliestChanged = insert(timer);
    
    //查看是否是最早的定时器
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
        /*
        最早的定时器需要resetTimerfd是因为在创建timerfd时，
        timerfd_create函数会自动启动一个定时器，
        并设置初始的超时时间。如果不对timerfd进行reset，
        会导致第一次的定时器事件发生时间早于预期
        */
    }
}


void TimerQueue::resetTimerfd(int timerfd_, TimeStamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    //itimerspec结构体
    //struct itimerspec {
    //    struct timespec it_interval; /* Interval for periodic timer */
    //    struct timespec it_value;    /* Initial expiration */
    //};
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    
    int64_t microseconds = expiration.microSecondsSinceEpoch()
                            - TimeStamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    
    ts.tv_sec = static_cast<time_t>(microseconds / TimeStamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % TimeStamp::kMicroSecondsPerSecond) * 1000);

    newValue.it_value = ts;
    //timerfd_settime()函数的第二个参数为0,表示相对时间，
    //第三个参数为新的超时时间，第四个参数为旧的超时时间
    if(::timerfd_settime(timerfd_, 0, &newValue, &oldValue) < 0)
    {
        LOG_ERROR << "timerfd_settime()";
    }
}





std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimeStamp now)//返回所有时间小于now的定时器，即超时的定时器
{
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    //set内部比较的是first,所以这里是根据now来比较,返回第一个大于等于now的迭代器
    std::copy(timers_.begin(), end, back_inserter(expired));
    //back_inserter:将元素添加到容器的尾部
    //将超时的定时器拷贝到expired中
    timers_.erase(timers_.begin(), end);
    //将超时的定时器从timers_中删除
    return expired;
}

void TimerQueue::handleRead()
{
    TimeStamp now(TimeStamp::now());
    ReadTimerfd(timerfd_);
    /*读取timerfd的数据其实是为了清空timerfd，让它重新计时。
    当一个timerfd被设置时，它会开始倒计时，当倒计时结束时，timerfd会变为可读状态，
    此时我们可以读取timerfd的数据，以清空它，让它重新开始倒计时。
    如果不读取timerfd的数据，它将一直处于可读状态，即使我们再次设置它，
    也不会触发新的计时，这样会导致timerfd无法正常工作。
    因此，每次处理timerfd事件时，都需要读取timerfd的数据，以便清空它。*/
    std::vector<Entry> expired = getExpired(now);
    for (std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        it->second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

void TimerQueue::reset(const std::vector<Entry>& expired, TimeStamp now)
{
    TimeStamp nextExpire;
    for(const Entry&it : expired)
    {
        if (it.second->repeat()){
            //如果是重复的定时器，则重新设置超时时间，并且重新插入到timers_中
            auto timer = it.second;
            timer->restart(TimeStamp::now());
            insert(timer);
        }else{
            delete it.second;
        }   
    }
    if (!timers_.empty())//如果timers_不为空,则重新设置timerfd的超时时间
    {
        resetTimerfd(timerfd_, timers_.begin()->second->expiration());
    }
}

//将定时器插入到timers_中
bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;//是否是最早的定时器
    TimeStamp when = timer->expiration();//定时器的超时时间
    TimerList::iterator it = timers_.begin();
    //it指向timers_的第一个元素，即最早的定时器
    if (it == timers_.end() || when < it->first)
    {    //如果timers_为空或者定时器的超时时间小于timers_中的第一个定时器的超时时间
        //则将定时器插入到timers_的最前面
        earliestChanged = true;
    }
    timers_.insert(std::make_pair(when, timer));
    return earliestChanged;
}

