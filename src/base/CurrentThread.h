#ifndef CURRENTTHREAD_H_
#define CURRENTTHREAD_H_

#include <unistd.h>
#include <sys/syscall.h>

// 定义namespace 作用是防止命名冲突
namespace CurrentThread
{
    extern __thread int t_cachedTid; // 线程id
    //__thread 表示该变量是线程局部存储的，每个线程都有一份独立实体。
    extern __thread char t_tidString[32];
    //线程id的字符串形式
    extern __thread int t_tidStringLength;
    //线程id的字符串长度
    void cacheTid(); // 获取线程id

    // 获取线程id,如果线程id为0,则调用cacheTid()函数获取线程id
    //__builtin_expect()函数用于告诉编译器某个条件分支的可能性
    //__builtin_expect(表达式,值)  表达式的值为真的可能性为值
    //__builtin_expect(表达式,0)   表达式的值为真的可能性为0
    //__builtin_expect(表达式,1)   表达式的值为真的可能性为1
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char *tidString() // for logging
    {
        return t_tidString;
    }

    inline int tidStringLength() // for logging
    {
        return t_tidStringLength;
    }
}

#endif