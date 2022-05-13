#include "hook.h"
#include "fiber.h"
#include "IOmanager.h"
#include "scheduler.h"
namespace zyx
{
    static thread_local bool t_hook_enable=false;
    #define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) 

    void hook_init() 
    {
        static bool is_inited = false;
        if(is_inited) 
        {
            return;
        }
        //等价于把原来的函数进行保存
        //sleep_f=(sleep_fun)dlsym(RTLD_NEXT, sleep);
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
            HOOK_FUN(XX);
        #undef XX
    }
    struct _HookIniter 
    {
        _HookIniter() 
        {
            hook_init();
        }
    };
    static _HookIniter s_hook_initer;
    bool is_hook_enable() 
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) 
    {
        t_hook_enable = flag;
    }
}



extern "C" 
{
    //等价于sleep_fun sleep_f=nullptr;
    #define XX(name) name ## _fun name ## _f = nullptr;
         HOOK_FUN(XX);
    #undef XX

   unsigned int sleep(unsigned int seconds) 
   {
        if(!zyx::t_hook_enable) {
            return sleep_f(seconds);
        }

        zyx::Fiber::ptr fiber = zyx::Fiber::GetThis();
        zyx::IOManager* iom = zyx::IOManager::GetThis();
        //bind:这里等价与iom->schedule(fiber,-1)
        iom->addTimer(seconds * 1000, std::bind((void(zyx::Scheduler::*)
                (zyx::Fiber::ptr, int thread))&zyx::IOManager::schedule
                ,iom, fiber, -1));
        zyx::Fiber::YieldToHold();//换出协程，等时间到后再换进来
        return 0;
    }

    int usleep(useconds_t usec) 
    {
        if(!zyx::t_hook_enable) {
            return usleep_f(usec);
        }
        zyx::Fiber::ptr fiber = zyx::Fiber::GetThis();
        zyx::IOManager* iom = zyx::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind((void(zyx::Scheduler::*)
                (zyx::Fiber::ptr, int thread))&zyx::IOManager::schedule
                ,iom, fiber, -1));
        zyx::Fiber::YieldToHold();
        return 0;
    } 
}