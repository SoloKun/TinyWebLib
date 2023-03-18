#include "Thread.h"

std::atomic_int32_t Thread::numCreated_(0);//线程索引

Thread::Thread(ThreadFunc func, const std::string& name)
    :started_(false),
    joined_(false),
    thread_(nullptr),
    tid_(0),
    func_(std::move(func)), //std::move()将左值转换为右值
    name_(name){
        setDefaultName();
    }

Thread::~Thread(){
    //如果线程启动且未加入，则分离线程
    if(started_ && !joined_){
        thread_->detach();
    }
}

void Thread::start(){
    started_ = true;
    //创建线程
    sem_t sem;//信号量
    sem_init(&sem, false, 0);//初始化信号量
    thread_ = std::shared_ptr<std::thread>(new std::thread([this, &sem](){
        tid_ = CurrentThread::tid();//获取线程id
        sem_post(&sem);//信号量加1
        func_();//执行线程函数
    }));
    //等待信号量,保证tid_被赋值
    sem_wait(&sem);//信号量减1
}


void Thread::join(){//设置为join，线程结束后会自动释放资源
//detach()分离线程，线程结束后不会自动释放资源
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num = numCreated_++;
    if(name_.empty()){
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

// int main(){
//     Thread t([](){
//         printf("hello world");
//     });
//     t.start();
//     t.join();
// }
