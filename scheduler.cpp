#include "scheduler.h"


namespace zyx
{
    static thread_local Scheduler* t_scheduler = nullptr; //当前的scheduler
    static thread_local Fiber* t_scheduler_fiber = nullptr;//当前的协程
    Scheduler::Scheduler(int thread_size,bool use_caller,std::string name)
    {
        ZYX_ASSERT(thread_size>0,"thread_size<0");
        if(use_caller)
        {
            Fiber::GetThis();
            --thread_size;

            ZYX_ASSERT(GetThis() == nullptr,"Scheduler has exited");
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));//创建主协程
            t_scheduler_fiber = m_rootFiber.get();
            m_rootThread = zyx::GetThreadId();//获得主线程id
        }
        else
        {
            m_rootThread=-1;
        }
        m_name=name;
        m_threadCount = thread_size;
    }
    Scheduler::~Scheduler()
    {
        if(GetThis() == this) 
        {
            t_scheduler = nullptr;
        }
    }
    Fiber* Scheduler::GetMainFiber() 
    {
        return t_scheduler_fiber;
    }
    Scheduler* Scheduler::GetThis() 
    {
        return t_scheduler;
    }

    void Scheduler::setThis() 
    {
        t_scheduler = this;
    }

    void Scheduler::start()
    {
        m_mutex.lock();  //初始化线程池，要初始化好
        if(!m_stopping)
        {
            return;
        }
        ZYX_ASSERT(m_threads.empty(),"threadpool is not empty");
        m_threads.resize(m_threadCount);
        for(size_t i = 0; i < m_threadCount; ++i) 
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , "thread_" + std::to_string(i)));
        }
        m_mutex.unlock(); 
    }

    void Scheduler::run()
    {
        setThis();
        if(zyx::GetThreadId() != m_rootThread) 
        {
            //getthis可以在没有协程被创建时，创建一个主协程
            t_scheduler_fiber = Fiber::GetThis().get();
        }
       // Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        FiberAndThread ft;
        Fiber::ptr cb_fiber;
        while(true)
        {
            //是否需要通知其他线程
            bool tickle_me = false;
            //把ft清空
            ft.reset();
            //接下来要在共有的协程链表中寻找可操作的协程，所以要加锁
            m_mutex.lock();
            auto it = m_fibers.begin();
            //取出一个要在当前线程执行的协程
             while(it != m_fibers.end())
             {
                 //如果该协程要求了要使用哪个线程，且当前不属于要求的线程，则遍历下一个
                if(it->thread != -1 && it->thread != zyx::GetThreadId())
                {
                    it++;
                    tickle_me=true;
                    continue;
                }
                ZYX_ASSERT(it->fiber|| it->cb,"FIBER no VALUE");
                //如果有fiber信息，但是该协程处于exec或者hold状态，则跳过
                if(it->fiber && (it->fiber->getState() == Fiber::EXEC||it->fiber->getState() == Fiber::HOLD))
                {
                    ++it;
                    continue;
                }
                //否则取出要执行的协程，并将其从列表中删除
                ft=*it;
                m_fibers.erase(it++);
                break;
             }
             //列表操作完成，解锁
            m_mutex.unlock();
            if(tickle_me) 
            {
                //tickle();//如果tickle为true，说明需要唤醒其他线程
            }
            //如果取出的协程有fiber，且不处于结束状态
            if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) 
            {
                ft.fiber->swapIn(); //执行协程

                if(ft.fiber->getState() == Fiber::READY)  //如果协程没有执行完成
                {
                    schedule(ft.fiber); //重新加入队列
                } 
                else if(ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT) 
                {
                    ft.fiber->m_state = Fiber::HOLD;
                    schedule(ft.fiber); //重新加入队列
                }
                ft.reset();
            }
            //如果取出的协程有cb，则我们包装一个协程执行
            else if(ft.cb)
            {
                if(cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else 
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                //执行协程
                cb_fiber->swapIn();
                //如果没有执行完(协程执行一半主动放弃处理机)
                if(cb_fiber->getState() == Fiber::READY) 
                {
                    schedule(cb_fiber);//再将该未执行完成部分加入列表
                    cb_fiber.reset(); //!!!!!清空cb_fiber
                }
                //如果执行完成
                else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) 
                {
                    cb_fiber->reset(nullptr);
                } 
                else 
                {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            //如果没有fiber与cb，或者有fiber但是该fiber已经处于结束状态
            else
            {
                //如果没有能处理的协程，按照不同功能实现不同的处理(idle虚函数)
                idle();
                //break;
            }
        }
    }
    void Scheduler::stop() 
    {
        for(auto i:m_threads)
        {
            i->join();
        }
    }
    //虚函数，之后按照不同功能做不同的实现
    void Scheduler::tickle()
    {
 
    }
    //虚函数，之后按照不同功能做不同的实现
    bool Scheduler::stopping()
    {
        return true;    
    }
    //虚函数，之后按照不同功能做不同的实现
    void Scheduler::idle()
    {
        //std::cout<<"idle"<<std::endl;
    }
}