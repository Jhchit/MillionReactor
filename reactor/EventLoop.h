#pragma once
#include "Epoll.h"
#include <functional>
#include <unistd.h>
#include <sys/syscall.h> 
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <memory>
#include <sys/timerfd.h> //定时器需要这个头文件
#include <map>
#include "Connection.h"
#include <mutex>
#include <atomic>

class Channel;
class Epoll;
class Connection;
using spConnection = std::shared_ptr<Connection>;

// 事件循环类。
class EventLoop
{
private:
    int  timetvl_;                                // 闹钟时间间隔，单位：秒。。
    int  timeout_;                                // Connection对象超时的时间，单位：秒。
    std::unique_ptr<Epoll> ep_;                   // 每个事件循环只有一个Epoll。
    std::function<void(EventLoop*)> epolltimeoutcallback_;
    std::function<void(int)> timercallback_;
    pid_t threadid_;                              //事件循环所在线程的id
    std::queue<std::function<void()>> taskqueue_; //事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex mutex_;                            // 任务队列同步的互斥锁。
    int wakeupfd_;
    std::unique_ptr<Channel> wakeupchannel_;
    int timerfd_;
    std::unique_ptr<Channel> timerchannel_;
    bool ismainloop_;                            // true-是主事件循环，false-是从事件循环。
    std::mutex mmutex_;                          // 保护conns_的互斥锁。
    std::map<int,spConnection> conns_;
    std::atomic_bool stop_;                      //当为true时，表示停止事件循环

public:
    EventLoop(bool ismainloop,int timetvl=30,int timeout=80);   // 在构造函数中创建Epoll对象ep_。
    ~EventLoop();                // 在析构函数中销毁ep_。

    void run();                      // 运行事件循环。
    void Stop();
    void updatachannel(Channel *ch);
    void removechannel(Channel *ch);
    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);
    void settimercallback(std::function<void(int)> fn); // 将被设置为TcpServer::removeconn()

    bool isinloopthread();   //判断当前线程是否为事件循环线程

    void queueinloop(std::function<void()> fn);
    void wakeup();          //用eventfd唤醒事件循环线程
    void handwakeup();      //事件循环线程被唤醒后执行的函数

    void handletimer();     // 闹钟响时执行的函数。

    void newconnection(spConnection conn); //把Connection对象保存在map容器中
};