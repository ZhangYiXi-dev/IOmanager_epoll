#ifndef HOOK_H
#define HOOK_H
#include <unistd.h>
#include <dlfcn.h>

namespace zyx
{
    /**
    * @brief 当前线程是否hook
    */
    bool is_hook_enable();
    /**
    * @brief 设置当前线程的hook状态
    */
    void set_hook_enable(bool flag);
}

extern "C"
{
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;
}
#endif
