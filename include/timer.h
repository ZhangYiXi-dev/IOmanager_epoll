#ifndef TIMER_H
#define TIMER_H
#include <memory>
#include <vector>
#include <set>
#include "zyx_thread.h"

namespace zyx
{
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer>
    {
    friend class TimerManager;
        public:
            /// 定时器的智能指针类型
            typedef std::shared_ptr<Timer> ptr;
        private:
            Timer(uint64_t ms, std::function<void()> cb,
                bool recurring, TimerManager* manager);
            Timer(uint64_t next);
        private:
            /// 是否循环定时器
            bool m_recurring = false;
            /// 执行周期
            uint64_t m_ms = 0;
            /// 精确的执行时间
            uint64_t m_next = 0;
            /// 回调函数
            std::function<void()> m_cb;
            /// 定时器管理器
            TimerManager* m_manager = nullptr;

        private:
            /**
             * @brief 定时器比较仿函数
             */
            struct Comparator 
            {
                /**
                 * @brief 比较定时器的智能指针的大小(按执行时间排序)
                 * @param[in] lhs 定时器智能指针
                 * @param[in] rhs 定时器智能指针
                 */
                bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
                {
                    if(!lhs && !rhs) 
                    {
                        return false;
                    }
                    if(!lhs) 
                    {
                        return true;
                    }
                    if(!rhs) 
                    {
                        return false;
                    }
                    if(lhs->m_next < rhs->m_next) 
                    {
                        return true;
                    }
                    if(rhs->m_next < lhs->m_next) 
                    {
                        return false;
                    }
                    return lhs.get() < rhs.get();
                }
            };
    };

    class TimerManager
    {
    friend class Timer;
        public:
            /**
             * @brief 构造函数
             */
            TimerManager();

            /**
             * @brief 析构函数
             */
            virtual ~TimerManager();
            /**
             * @brief 添加定时器
             * @param[in] ms 定时器执行间隔时间
             * @param[in] cb 定时器回调函数
             * @param[in] recurring 是否循环定时器
             */
            Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                                ,bool recurring = false);
            /**
             * @brief 到最近一个定时器执行的时间间隔(毫秒)
             */
            uint64_t getNextTimer();
            /**
             * @brief 获取需要执行的定时器的回调函数列表
             * @param[out] cbs 回调函数数组
             */
            void listExpiredCb(std::vector<std::function<void()> >& cbs);
            /**
             * @brief 当有新的定时器插入到定时器的首部,执行该函数
             */
            virtual void onTimerInsertedAtFront() = 0;
        private:
            /// Mutex
            RWMutex m_mutex;
             /// 定时器集合
            std::set<Timer::ptr, Timer::Comparator> m_timers;
            /// 是否触发onTimerInsertedAtFront
            bool m_tickled = false;
            /// 上次执行时间
            uint64_t m_previouseTime = 0;
    };
}
#endif