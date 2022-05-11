#include "IOmanager.h"

namespace zyx
{
    void IOManager::FdContext::triggerEvent(IOManager::Event event) 
    {
        //SYLAR_LOG_INFO(g_logger) << "fd=" << fd
        //    << " triggerEvent event=" << event
        //    << " events=" << events;
        //ZYX_ASSERT(events & event);
        //if(SYLAR_UNLIKELY(!(event & event))) {
        //    return;
        //}
        //events = (Event)(events & ~event);
        EventContext& ctx = getContext(event);
        if(ctx.cb) 
        {
            ctx.scheduler->schedule(ctx.cb);
        } 
        else 
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        return;
    }
    void IOManager::FdContext::triggerEvent(IOManager::Event event,int fd) 
    {
        EventContext& ctx = getContext(event);
        if(ctx.cb) 
        {
            ctx.scheduler->schedule(ctx.cb,fd,-1);
        } 
        else 
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        return;
    }
    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) 
    {
        switch(event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                ZYX_ASSERT(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name) 
    :Scheduler(threads,use_caller,name)    //子类给父类构造函数传递参数
    {
        m_epfd = epoll_create(50000);//创建epoll监听红黑树
        ZYX_ASSERT(m_epfd > 0,"epoll create erro");
        
        int rt = pipe(m_tickleFds);//创建一个管道用来唤醒线程
        ZYX_ASSERT(!rt,"pipe create erro");

        //将管道的读事件挂上监听树
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET; //边沿触发、读监听
        event.data.fd=m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK); //O_NONBLOCK：非阻塞模式
        ZYX_ASSERT(!rt,"fcntl error");

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        ZYX_ASSERT(!rt,"epoll_ctl error");

        contextResize(32);
        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i = 0; i < m_fdContexts.size(); ++i) 
        {
            if(m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }
    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        for(size_t i = 0; i < m_fdContexts.size(); ++i) 
        {
            if(!m_fdContexts[i]) 
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }
    void IOManager::FdContext::resetContext(EventContext& ctx) 
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    //添加事件
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext* fd_ctx = nullptr;
        m_rw_mutex.rdlock();
        if((int)m_fdContexts.size() > fd) //判断事件是否还能装得下
        {
            fd_ctx = m_fdContexts[fd];
            m_rw_mutex.unlock();
        } 
        else //装不下要进行1.5倍的扩容
        {
            m_rw_mutex.unlock();
            m_rw_mutex.wrlock();
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
            m_rw_mutex.unlock();
        }
        fd_ctx->mutex.lock();
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;
        
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt)
        {
            printf("epoll_ctl error %d:%s\r\n",errno,strerror(errno));
            ZYX_ASSERT(false,"epoll_ctl error")
            return -1;
        }
        ++m_pendingEventCount;
        fd_ctx->events=(Event)(fd_ctx->events | event);

        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        ZYX_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb," ");
        event_ctx.scheduler = Scheduler::GetThis();
        ZYX_ASSERT(event_ctx.scheduler
                    ,"scheduler nullptr");
        if(cb) 
        {
            event_ctx.cb.swap(cb);
        } 
        else 
        {
            event_ctx.fiber = Fiber::GetThis();
            //ZYX_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC
            //           ," ");
        }
        fd_ctx->mutex.unlock();
        return 0;
    }
    bool IOManager::delEvent(int fd, Event event)
    {
        m_rw_mutex.rdlock();
        if((int)m_fdContexts.size() <= fd) 
        {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        m_rw_mutex.unlock();
        fd_ctx->mutex.lock();
        if(fd_ctx->events==NONE)
        {
            return false;
        }
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt) 
        {
            std::cout<<"epoll_ctl error "<<errno<<" "<<strerror(errno)<<std::endl;
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        fd_ctx->mutex.unlock();
        return true;
    }
   
    //通知空闲线程来处理
    void IOManager::tickle() 
    {
        // if(!hasIdleThreads()) 
        // {
        //     return;
        // }
       // int rt = write(m_tickleFds[1], "T", 1);
        //ZYX_ASSERT(rt == 1,"write error");
    }
    bool IOManager::stopping(uint64_t& timeout) 
    {
        timeout = getNextTimer();
        return timeout == ~0ull
            && m_pendingEventCount == 0
            && Scheduler::stopping();

    }
    bool IOManager::stopping() 
    {
        // uint64_t timeout = 0;
        // return stopping(timeout);
        return true;
    }
    void IOManager::idle()
    {
        const uint64_t MAX_EVNETS = 256;
        //接收事件被激活的epoll事件
        epoll_event* events = new epoll_event[MAX_EVNETS]();
        int rt=0;
        uint64_t next_timeout = 0;
        stopping(next_timeout);
        do {
            static const int MAX_TIMEOUT = 100;
            if(next_timeout != ~0ull) 
            {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } 
            else 
            {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            if(rt < 0 && errno == EINTR) 
            {
            } 
            else 
            {
                break;
            }
        } while(true);
        //rt = epoll_wait(m_epfd, events, MAX_EVNETS, 1);
        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) 
        {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }
        for(int i = 0; i < rt; ++i) 
        {
            //std::cout<<"having event "<<rt<<std::endl;
            epoll_event& event = events[i];
            //如果是tickle发出的，读出来就可以了，只是一个激活
            if(event.data.fd == m_tickleFds[0]) 
            {
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
           // std::cout<<"real event "<<rt<<std::endl;
            //否则为真正的事件
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            fd_ctx->mutex.lock();
            if(event.events & (EPOLLERR | EPOLLHUP)) 
            {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            if(event.events &EPOLLRDHUP)
            {
                continue;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) 
            {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) 
            {
                real_events |= WRITE;
            }
            if((fd_ctx->events & real_events) == NONE) 
            {
                //std::cout<<"none "<<rt<<std::endl;
                continue;
            }
            
            if(real_events & READ) 
            {
                //std::cout<<"read event "<<rt<<std::endl;
                fd_ctx->triggerEvent(READ,fd_ctx->fd);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) 
            {
                //std::cout<<"write event "<<rt<<std::endl;
                fd_ctx->triggerEvent(WRITE,fd_ctx->fd);
                --m_pendingEventCount;
            }
            fd_ctx->mutex.unlock();
        }
    }
    void IOManager::onTimerInsertedAtFront() 
    {
        tickle();
    }
}