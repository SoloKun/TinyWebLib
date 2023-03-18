#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <string>
#include <vector>
#include <memory>
#include <functional>


class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:

    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }

    const std::string &name() const { return name_; }

private:
    EventLoop *baseLoop_; //主线程的loop
    std::string name_;//线程池的名字
    bool started_;
    int numThreads_;
    int next_;//下一个要分配的线程
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    //unique_ptr是一个智能指针，
    //它的特点是：只能有一个指针指向它，当这个指针被销毁时，
    //它所指向的对象也会被销毁。
    std::vector<EventLoop *> loops_;
};







#endif