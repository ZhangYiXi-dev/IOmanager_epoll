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

//时间 线程id-线程名称 协程id  自定义信息
zyx::Logger::ptr log_main = (new zyx::LoggerManager(zyx::LogLevel::Level::DEBUG, true, false))->Getlogger(); 
int pfd[2];

zyx::Mutex mutex;

void test_read()
{
    char b[1024];
    mutex.lock();
    int len=read(pfd[0],b,sizeof(b));
    printf("%s\n",b);
    ZYX_LOG_DEBUG(log_main,"read");
    mutex.unlock();    
}
void test_write()
{
    ZYX_LOG_DEBUG(log_main,"write");
}
int main()
{
  
  pipe(pfd);
  zyx::IOManager::ptr io(new zyx::IOManager(10));
  io->setThis();
  io->addEvent(pfd[0], zyx::IOManager::Event::READ, test_read);
  io->addEvent(pfd[1], zyx::IOManager::Event::WRITE, test_write);
  
  char buf[]="aaaa";
  for(int i=0;i<100;i++)
  {
     mutex.lock();
    write(pfd[1], buf, sizeof(buf));
    mutex.unlock();
    usleep(1000);
  }
 
  //sleep(1);
  //close(pfd[0]);
  //io->stop();

}