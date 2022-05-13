#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include "common.h"
#include "zyx_thread.h"
#include "fiber.h"
#include "hook.h"

namespace zyx
{
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        //如果是false则没设置主协程，否则会设置主协程
        Scheduler(int thread_size=1,bool use_caller=false,std::string name=""); 
        ~Scheduler();
         /**
         * @brief 返回当前协程调度器
         */
        static Scheduler* GetThis();
        /**
         * @brief 返回当前协程调度器的调度协程
         */
        static Fiber* GetMainFiber();
        /**
         * @brief 返回当前被触发的fd
         */
        static int Getnowfd();
        /**
        * @brief 设置当前的协程调度器
        */
        void setThis();
         /**
         * @brief 协程调度函数
         */
        void run();

        //调度器启动
        void start();
        /**
         * @brief 停止协程调度器
         */
        void stop();
        /**
         * @brief 调度协程
         * @param[in] fc 协程或函数
         * @param[in] thread 协程执行的线程id,-1标识任意线程
         */
        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) 
        {
            bool need_tickle = false;
            {
                m_mutex.lock();
                //std::cout<<"schedu "<<std::endl;
                need_tickle = scheduleNoLock(fc, thread);
                m_mutex.unlock();
            }

            if(need_tickle) {
                tickle();
            }
        }
        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int fd,int thread = -1) 
        {
            bool need_tickle = false;
            {
                m_mutex.lock();
                need_tickle = scheduleNoLock(fc, fd,thread);
                m_mutex.unlock();
            }

            if(need_tickle) 
            {
                tickle();
            }
        }
         /**
         * @brief 批量调度协程
         * @param[in] begin 协程数组的开始
         * @param[in] end 协程数组的结束
         */
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) 
        {
            bool need_tickle = false;
            {
                m_mutex.lock();
                while(begin != end) 
                {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
                m_mutex.unlock();
            }
            if(need_tickle) 
            {
                tickle();
            }
        }
    private:
           /**
         * @brief 协程/函数/线程组
         */
        struct FiberAndThread {
            /// 协程
            Fiber::ptr fiber;
            /// 协程执行函数
            std::function<void()> cb;
            /// 线程id
            int thread;
            ///触发的文件描述符
            int fd=0;

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }
        FiberAndThread(std::function<void()> f, int fd,int thr)
            :cb(f), thread(thr),fd(fd) {
        }
        /**
         * @brief 无参构造函数
         */
        FiberAndThread()
            :thread(-1) {
        }

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) 
    {
        bool need_tickle = m_fibers.empty();
        //std::cout<<"schedunolock "<<std::endl;
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int fd,int thread) 
    {
        bool need_tickle = m_fibers.empty();
        //std::cout<<"schedunolock "<<std::endl;
        FiberAndThread ft(fc, fd,thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
    protected:
        /**
         * @brief 通知协程调度器有任务了
         */
        virtual void tickle();
        /**
         * @brief 返回是否可以停止
         */
        virtual bool stopping();
        /**
         * @brief 协程无任务可调度时执行idle协程
         */
        virtual void idle();
    private:
        Mutex m_mutex; //互斥锁
        int m_threadCount=0; //线程数量
        std::vector<Thread::ptr> m_threads; //线程池
        std::list<FiberAndThread> m_fibers; //协程队列
        Fiber::ptr m_rootFiber; //调度其他协程的主协程
        bool m_stopping=true;
        pid_t m_rootThread = 0;/// 主线程id(use_caller)
        std::string m_name; /// 协程调度器名称
    };
}
#endif