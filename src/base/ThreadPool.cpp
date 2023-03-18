#include "ThreadPool.h"

ThreadPool::ThreadPool(const std::string& name)
    :mutex_(),
    cond_(),
    name_(name),
    running_(false)
{
}

ThreadPool::~ThreadPool(){
    if(running_){
        stop();
    }
    for(auto& thread : threads_){
        //join和detach都可以
        thread->join();//join 更好排查问题
    }
}

void ThreadPool::start(){
    running_ = true;
    threads_.reserve(threadSize_);
    for(int i = 0; i < threadSize_; ++i){
        char id[32];
        snprintf(id, sizeof id, "%d", i+1);
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this), name_+id));
        //创建线程，线程执行函数为runInThread
        
        //std::bind()将函数绑定到对象上
        //std::bind(&ThreadPool::runInThread, this)相当于
        //std::bind(&ThreadPool::runInThread, this, std::placeholders::_1, std::placeholders::_2, ...)
        
        threads_[i]->start();
    }
    //不创建线程，直接在主线程中执行
    if (threadSize_ == 0 && threadInitCallback_){
        threadInitCallback_();
    }
}

void ThreadPool::stop(){
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        cond_.notify_all();
    }
}

//返回任务队列的长度
size_t ThreadPool::queueSize() const{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::addTask(ThreadFunction& task){
    if(threads_.empty()){
        task();//直接在主线程中执行
    }
    else{
        std::lock_guard<std::mutex> lock(mutex_);
        //lock_guard是一个RAII机制的智能指针，它的构造函数会自动加锁，析构函数会自动解锁
        //lock_guard的构造函数接受一个mutex对象，它的析构函数会自动调用mutex的unlock()函数
        //unique_lock是一个RAII机制的智能指针，它的构造函数会自动加锁，析构函数会自动解锁
        //unique_lock的构造函数接受一个mutex对象，它的析构函数会自动调用mutex的unlock()函数
        //区别在于unique_lock可以在构造时加锁，也可以在构造时不加锁，然后在后面的代码中加锁
        //unique_lock可以随时加锁和解锁，lock_guard只能在构造时加锁，析构时解锁
        //unique_lock和condition_variable配合使用时，可以随时加锁和解锁
        queue_.push_back(task);
        cond_.notify_one();
    }
}


//执行任务队列中的任务
void ThreadPool::runInThread(){
    try{
        if(threadInitCallback_){//线程初始化回调函数
            threadInitCallback_();
        }
        ThreadFunction task;
        while(running_){
            {
                std::unique_lock<std::mutex> lock(mutex_);
                while(queue_.empty()){
                    if(!running_){
                        return;
                    }
                    cond_.wait(lock);//等待任务队列中有任务，或者线程池停止
                }
                task = queue_.front();
                queue_.pop_front();
            }
            if(task!=nullptr){
                task();
            }
        }
    }
    catch(const std::exception& ex){
        LOG_FATAL << "exception caught in ThreadPool " << name_
                  << " " << ex.what();
        abort();
    }
    catch(...){
        LOG_FATAL << "unknown exception caught in ThreadPool " << name_;
        abort();
    }
}



// void print()
// {
//     printf("tid=%d\n", CurrentThread::tid());
// }

// void printString(const std::string& str)
// {
//     LOG_INFO << str;
//     usleep(100*1000);
// }

// void test(int maxSize)
// {
//     LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
//     ThreadPool pool("MainThreadPool");
//     pool.setThreadSize(5);
//     pool.start();

//     LOG_WARN << "Adding";
//     std::function<void()> task(std::bind(print));
//     pool.addTask(task);
//     pool.addTask(task);
//     for (int i = 0; i < 100; ++i) {
//         char buf[32];
//         snprintf(buf, sizeof(buf), "task %d", i);
//         std::function<void()> task(std::bind(printString, std::string(buf)));
//         pool.addTask(task);
//     }
//     LOG_WARN << "Done";
//     
//     pool.stop();
// }

// int main()
// {
//     test(0);
//     test(1);
//     test(5);
//     test(10);
//     test(50);
//     return 0;
// }