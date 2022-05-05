/***
 * 调度器
 * 实现功能：epoll
*/
#include <iostream>
#include <vector>
#include <time.h>
#include <assert.h>
#include "zyx_thread.h"
#include "measure_time.h"
#include "common.h"
#include "log.h"
#include "fiber.h"
#include "scheduler.h"
#include "IOmanager.h"
#include "mutex.h"
#include "Socket.h"

//时间 线程id-线程名称 协程id  自定义信息
zyx::Logger::ptr log_main = (new zyx::LoggerManager(zyx::LogLevel::Level::DEBUG, false, true))->Getlogger(); 
int pfd[2];
std::string msg="hello";
int sum=0;
void rd_cb(void * param)
{
    int* fd=(int*)param;
    char buf[1024];
    int flag=read(*fd,buf,sizeof(buf)); 
    if(flag==-1)
    {
        return;
    }
    if(flag==0)
    {
        //epoll_ctl (m_epfd,EPOLL_CTL_DEL,*fd,&event);
        //close(*fd);
        return;
    }
    write(*fd,msg.c_str(),msg.size());
    std::string str;
    str.append(buf);
    ZYX_LOG_DEBUG(log_main,std::to_string(*fd)+" "+str);
}
void wr_cb(void * param)
{
    ZYX_LOG_DEBUG(log_main,"write");
}

int main()
{
    zyx::Socket::ptr p(new zyx::Socket(12160,50,rd_cb,wr_cb));
    p->Setthis();
    while(1)
    {

    }
            
}