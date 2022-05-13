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
#include "hook.h"

//时间 线程id-线程名称 协程id  自定义信息
zyx::Logger::ptr log_main = (new zyx::LoggerManager(zyx::LogLevel::Level::DEBUG, true, true))->Getlogger();
void test_sleep()
{
    zyx::IOManager iom(1);
    iom.setThis();
    iom.schedule([](){
        sleep(2);
        ZYX_LOG_DEBUG(log_main,"sleep 2")
    });
    iom.schedule([](){
        sleep(3);
        ZYX_LOG_DEBUG(log_main,"sleep 3")
    });
}
int main()
{
    test_sleep();
    return 0;
}