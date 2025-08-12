#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>
#include "Socket.h"

class EventLoop;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection:public std::enable_shared_from_this<Connection>
{
private:
    EventLoop *loop_;                               // Connection对应的事件循环，在构造函数中传入。
    std::unique_ptr<Socket> clientsock_;                            // 与客户端通信的Socket
    std::unique_ptr<Channel>clientchannel_;                        // Connection对应的channel，在构造函数中创建。
    Buffer inputbuffer_;
    Buffer outputbuffer_;
    std::atomic_bool disconnect_;                   //客户端是否已经断开，若断开设为true

    std::function<void(spConnection)> closecallback_;
    std::function<void(spConnection)> errorcallback_;
    std::function<void(spConnection,std::string&)> onmessagecallback_;
    std::function<void(spConnection)> sendcompletecallback_;
    std::vector<char> send_buffer_;
    Timestamp lasttime_;    // 时间戳，创建Connection对象时为当前时间，每接收到一个报文，把时间戳更新为当前时间。

public:
    Connection(EventLoop *loop,std::unique_ptr<Socket>clientsock,int sep);
    ~Connection();

    int fd() const;                         // 返回fd_成员。
    std::string ip() const;                 // 返回ip_成员。
    uint16_t port() const;                  // 返回port_成员。
    
    void onmessage();

    void closecallback();                   // TCP连接关闭（断开）的回调函数，供Channel回调。
    void errorcallback();                   // TCP连接错误的回调函数，供Channel回调。
    void writecallback();                   // 处理写事件的回调函数，Channel回调         

    void setclosecallback(std::function<void(spConnection)> fn);
    void seterrorcallback(std::function<void(spConnection)> fn);
    void setonmessagecallback( std::function<void(spConnection,std::string&)> fn);
    void setsendcompletecallback( std::function<void(spConnection)> fn);

    //发送数据，不管在任何线程中，都是调用此函数发送数据。
    void send(const char *data,size_t size);
    // 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
    void sendinloop(const char *data,size_t size);
    // 判断连接是否超时（空闲太久）。
    bool timeout(time_t now,int val);
};