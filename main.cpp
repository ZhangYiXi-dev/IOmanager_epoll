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
#include "timer.h"

//时间 线程id-线程名称 协程id  自定义信息
zyx::Logger::ptr log_main = (new zyx::LoggerManager(zyx::LogLevel::Level::DEBUG, true, false))->Getlogger();
int pfd[2];
std::string msg="hello";
int sum=0;
void rd_cb(void * param)
{
    int* fd=(int*)param;
    char buf[1024];
    bool flag=zyx::Socket::Revmsg(*fd,buf,sizeof(buf));
    if(!flag)
    {
        return;
    }
    write(*fd,msg.c_str(),msg.size());
    std::string str;
    str.append(buf);
    ZYX_LOG_DEBUG(log_main,std::to_string(*fd)+" "+str);
    sleep(1);
}
void wr_cb(void * param)
{
    ZYX_LOG_DEBUG(log_main,"write");
}

zyx::Timer::ptr s_timer; 
void test_timer() 
{
    zyx::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        ZYX_LOG_INFO(log_main,"hello timer i="+std::to_string(i)) ;
        i++;
    },true);
}
void test_socket()
{
    zyx::Socket::ptr p(new zyx::Socket(12160,100,rd_cb,wr_cb));
    p->Setthis();
}
int main()
{
   test_timer(); 
   //test_socket();
   while(1)
   {

   }       
}