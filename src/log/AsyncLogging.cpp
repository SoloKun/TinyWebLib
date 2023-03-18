#include "AsyncLogging.h"

AsyncLogging::AsyncLogging(const std::string &basename,
                           off_t rollSize,
                           int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

//将日志写入缓冲区，由前端线程调用，前端将该函数作为out的回调函数
void AsyncLogging::append(const char *logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {//当前缓冲区不够用,将其放入缓冲区数组中
        buffers_.push_back(std::move(currentBuffer_));
        //unique_ptr的移动语义，将currentBuffer_的所有权转移给buffers_中的元素
        //移动后currentBuffer_的所有权为空
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else//备用缓冲区为空,则创建一个新的缓冲区
        {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        cond_.notify_one();
        //后端线程只有一个，若有多个后端线程，需要使用notify_all
    }
}


//如果异步线程崩溃，会导致日志丢失，因此需要在析构函数中调用stop()函数，确保日志写入文件
void AsyncLogging::threadFunc()//线程函数,将缓冲区数组中的数据写入文件
{
    assert(running_ == true);
    LogFile output(basename_, rollSize_, false);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;//缓冲区数组
    buffersToWrite.reserve(16);
    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())//若缓冲区数组为空,则等待
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
                //等待flushInterval_秒
            }
            buffers_.push_back(std::move(currentBuffer_));//将当前缓冲区放入缓冲区数组中
            currentBuffer_ = std::move(newBuffer1);//将备用缓冲区赋值给当前缓冲区
            buffersToWrite.swap(buffers_);//将缓冲区数组赋值给buffersToWrite
            if (!nextBuffer_)//若备用缓冲区为空,则将新缓冲区赋值给备用缓冲区
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());
        //将缓冲区数组中的数据写入文件
        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }
        //将buffersToWrite中的数据写入文件后,将其清空
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}

// #include "Logging.h"
// int main(){
//     Logger::setLogLevel(Logger::DEBUG);
//     LOG_TRACE << "trace";
//     LOG_DEBUG << "debug";
//     LOG_INFO << "info";
//     LOG_WARN << "warn";
//     LOG_ERROR << "error";

//     AsyncLogging log("test", 1024*1024*10, 3);
//     log.start();
//     for(int i = 0; i < 100000; i++){
//         LOG_INFO << "info";
//     }
//     log.stop();
//     return 0;
// }