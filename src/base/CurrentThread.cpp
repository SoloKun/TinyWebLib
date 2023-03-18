#include "CurrentThread.h"

namespace CurrentThread{
    __thread int t_cachedTid = 0;//线程id默认为0，表示未获取线程id
    void cacheTid(){
        if(t_cachedTid == 0){
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
            //syscall()函数用于调用内核的系统调用,获取线程id,并赋值给t_cachedTid

        }
    }
    
}