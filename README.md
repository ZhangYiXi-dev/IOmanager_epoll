# IOmanager_epoll_socket
扩展协程调度器，实现一个io的调度器，封装epoll，实现一个socket通信
其中的定时器是通过epoll实现的

简单记录
```cpp
一：
协程的回调是无参数的，如何知道是哪个文件描述符触发的事件？
解决：协程是一个特殊函数，在线程中相当于是串行的，可以创建一个static thread_local的变量保存
当前fd。该fd在swapin前赋值。且类中写一个getfd的函数，方便系统调用

二：
收到sigpipe信号、epoll_ctl错误码2(no such file)
解决：这种情况主要是write了已经关闭的fd，add的fd已经存在，解决方法是在收到关闭信号后要将监听
从红黑树取下，然后close fd。同时要及时del已经关闭的fd，不然大量短连接下会使fd很快用尽。

三：
添加事件和删除事件要加锁

四：
gprof 性能测试
首先要加-g -pg参数，其次注意要程序正常退出(return、exit(0))才会生成，多线程的话要重写

未完待续.......
```

