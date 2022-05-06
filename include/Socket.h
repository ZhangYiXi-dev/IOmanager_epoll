#ifndef SOCKET_H
#define SOCKET_H
#include <memory>
#include "noncopyable.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "IOmanager.h"
namespace zyx
{
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
    {
        public:
            typedef std::shared_ptr<Socket> ptr;
            typedef std::weak_ptr<Socket> weak_ptr;

            /**
             * @brief Socket类型
             */
            enum Type 
            {
                /// TCP类型
                TCP = SOCK_STREAM,
                /// UDP类型
                UDP = SOCK_DGRAM
            };

            /**
             * @brief Socket协议簇
             */
            enum Family 
            {
                /// IPv4 socket
                IPv4 = AF_INET,
                /// IPv6 socket
                IPv6 = AF_INET6,
                /// Unix socket
                UNIX = AF_UNIX,
            };
            Socket(int port,size_t threads,std::function<void(void*)> rd_cb,
                    std::function<void(void*)> wr_cb,Type type=TCP,Family family=IPv4,
                   bool use_caller=false, const std::string& name="");
            ~Socket();
            void Setthis();
            static Socket::ptr GetThis();
            static bool delEvent(int fd,IOManager::Event event);
            static bool Revmsg(int fd,char *buf,int size);
        private:
            static void s_rd_cb();
            static void s_wr_cb();
            static void Accept();
   
        public:
           // void addEvent(Socket_fd fd);
        private:
            IOManager::ptr m_io;
            Type m_type;
            Family m_family;
            int m_port;
            std::function<void(void*)> m_rd_cb;
            std::function<void(void*)> m_wr_cb;
            
            int m_listenfd;

    };

    // class Socket_fd: public std::enable_shared_from_this<Socket_fd>
    // {
    //     public:
    //         typedef std::shared_ptr<Socket_fd> ptr;
    //         Socket_fd(int fd,IOManager::Event type,std::function<void()> rd_cb,std::function<void()> wr_cb);
    //         ~Socket_fd();
    //         int Getfd(){return m_fd;}
    //         IOManager::Event GetType(){return m_type;}
    //     public:
    //         static void m_rd_cb();
    //         static void m_wr_cb();
    //     private:
    //         int m_fd;
    //         IOManager::Event m_type;
            
    // };
}
#endif