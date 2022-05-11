#include "timer.h"

namespace zyx
{
    Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
     :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) 
    {
        m_next = zyx::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
    :m_next(next) 
    {
    }

    TimerManager::TimerManager() 
    {
        m_previouseTime = zyx::GetCurrentMS();
    }

    TimerManager::~TimerManager() 
    {
    }
    void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) 
    {
        uint64_t now_ms = zyx::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        
        m_mutex.rdlock();
        if(m_timers.empty()) 
        {
            return;
        }
        m_mutex.unlock();
        
        m_mutex.wrlock();
        Timer::ptr now_timer(new Timer(now_ms));
        auto it = m_timers.lower_bound(now_timer);
        while(it != m_timers.end() && (*it)->m_next == now_ms) 
        {
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for(auto& timer : expired) 
        {
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring) {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            } 
            else 
            {
                timer->m_cb = nullptr;
            }
        }
    }
    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                                ,bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        m_mutex.wrlock();
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if(at_front) 
        {
            m_tickled = true;
        }
        m_mutex.unlock();
        if(at_front) 
        {
            onTimerInsertedAtFront();//tickel一下，不然如果没有别的事件发生，则无法利用epoll_wait定时
        }
        return timer;
    }
    uint64_t TimerManager::getNextTimer()
    {
        m_mutex.rdlock();
        m_tickled = false;
        if(m_timers.empty()) 
        {
            return ~0ull;
        }
        const Timer::ptr& next = *m_timers.begin();
        m_mutex.unlock();
        uint64_t now_ms = zyx::GetCurrentMS();
        if(now_ms >= next->m_next) 
        {
            return 0;
        } 
        else 
        {
            return next->m_next - now_ms;
        }
    }
}