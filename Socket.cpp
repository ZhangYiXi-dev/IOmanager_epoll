#include "Socket.h"

namespace zyx
{
    static Socket::ptr now_Socket=nullptr;
    
    Socket::Socket(int port,size_t threads, std::function<void(void*)> rd_cb,
                    std::function<void(void*)> wr_cb,Type type,Family family,
                   bool use_caller, const std::string& name)
    {
        m_rd_cb=rd_cb;
        m_wr_cb=wr_cb;
        m_port=port;
        m_type=type;
        m_family=family;

        int rt;
        int listenfd = socket(family, type, 0);
        if(listenfd<0)
        {
            ZYX_ASSERT(false,"socket error");
        }
        struct sockaddr_in servaddr;
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(m_port);

        rt=bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if(rt<0)
        {
            ZYX_ASSERT(false,"bind error");
        }

        rt=listen(listenfd, 20);
        if(rt<0)
        {
            ZYX_ASSERT(false,"listen error");
        }
        m_listenfd=listenfd;
        m_io=zyx::IOManager::ptr(new zyx::IOManager(threads,use_caller,name));
        m_io->setThis();
        m_io->addEvent(listenfd,IOManager::Event::READ,Accept);
        
    }
    Socket::~Socket()
    {
        close(m_listenfd);
        m_io->stop();
    }
    Socket::ptr Socket::GetThis()
    {
        return now_Socket;
    }

    void Socket::Setthis()
    {
        now_Socket=shared_from_this();
    }
//zyx::Logger::ptr log_so = (new zyx::LoggerManager(zyx::LogLevel::Level::DEBUG, true, false))->Getlogger(); 

    void Socket::Accept()
    {
        //ZYX_LOG_DEBUG(log_so,std::to_string(now_Socket->m_listenfd)+"accept");
        struct sockaddr_in sin;
        socklen_t len = sizeof(struct sockaddr_in);
        bzero(&sin, len);
    
        int confd = accept(now_Socket->m_listenfd, (struct sockaddr*)&sin, &len);
        if(confd<0)
        {
            ZYX_ASSERT(false,"accept error");
        }
        now_Socket->m_io->addEvent(confd,IOManager::Event::READ,s_rd_cb);
    }
    void Socket::s_rd_cb()
    {
        int *n_fd=new int;
        *n_fd=Scheduler::Getnowfd();
        now_Socket->m_rd_cb((void*)n_fd);
        delete n_fd;
    }
    void Socket::s_wr_cb()
    {
        int *n_fd=new int;
        *n_fd=Scheduler::Getnowfd();
        now_Socket->m_wr_cb((void*)n_fd);
        delete n_fd;
    }
    bool Socket::delEvent(int fd,IOManager::Event event)
    {
        return now_Socket->m_io->delEvent(fd,event);
    }
    bool Socket::Revmsg(int fd,char *buf,int size)
    {
        int flag=read(fd,buf,1024); 
        if(flag==-1)
        {
            return false;
        }
        if(flag==0)
        {
            zyx::Socket::delEvent(fd,zyx::IOManager::Event::READ);
            close(fd);
            return false;
        }
        return true;
    }
    // Socket_fd::Socket_fd(int fd,IOManager::Event type,std::function<void()> rd_cb,std::function<void()> wr_cb)
    // {
    //     m_fd=fd;
    //     m_type=type;
        
    // }
    // void Socket_fd::m_rd_cb()
    // {
        
    // }
}