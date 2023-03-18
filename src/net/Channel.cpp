#include "Channel.h"
#include "EventLoop.h"


const int Channel::kNoneEvent = 0; //空闲事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
//读事件,EPOLLPRI表示对应的文件描述符有紧急的数据可读,
//这里应该是有带外数据到来,带外数据是指优先级高的数据,通常是通过TCP紧急指针来发送的
//EPOLLIN表示对应的文件描述符可以读(包括对端SOCKET正常关闭)
const int Channel::kWriteEvent = EPOLLOUT; //写事件,
//EPOLLOUT表示对应的文件描述符可以写

Channel::Channel(EventLoop* loop,int fd)
    :loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    tied_(false)
{
}

//设置tie_指向一个对象，该对象的生命周期由tie_管理
// 在TcpConnection建立得时候会调用,这样就可以保证TcpConnection的生命周期
void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

Channel::~Channel(){
    if(loop_->isInLoopThread()){
        assert(!loop_->hasChannel(this));
    }
}

void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::remove(){
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

void Channel::handleEvent(TimeStamp receiveTime){
    if(tied_){//被tie_管理。这时候channel的生命周期由tie_管理，如果tie_失效，说明channel已经失效
        std::shared_ptr<void>guard = tie_.lock();
        //tie_.lock()返回一个shared_ptr,如果tie_失效，返回的shared_ptr为空
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(TimeStamp receiveTime){
    //事件处理
    LOG_DEBUG << "Channel::handle_event() revents_ = " << revents_;
    
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallback_) closeCallback_();
    }
    if(revents_ & EPOLLERR){
        LOG_ERROR << "Channel::handle_event() EPOLLERR"<< this->fd();
        if(errorCallback_) errorCallback_();
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        LOG_DEBUG << "Channel::handle_event() EPOLLIN | EPOLLPRI";
        if(readCallback_) readCallback_(receiveTime);
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_) writeCallback_();
    }
       
}