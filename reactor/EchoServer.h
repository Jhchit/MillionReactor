#pragma once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include <iostream>
#include "ThreadPool.h"

class EchoServer
{
private:
    TcpServer tcpserver_;
    ThreadPool threadpool_;
public:
    EchoServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum = 5,int sep = 0);
    ~EchoServer();

    void Start();
    void Stop();

    void HandleNewConnection(spConnection conn);
    void HandleClose(spConnection conn);  // 关闭客户端的连接，在Connection类中回调此函数。 
    void HandleError(spConnection conn);  // 客户端的连接错误，在Connection类中回调此函数。
    void HandleMessage(spConnection conn,std::string& message);     // 处理客户端的请求报文，在Connection类中回调此函数。
    void HandleSendComplete(spConnection conn);     // 数据发送完成后，在Connection类中回调此函数。
    //void HandleTimeOut(EventLoop *loop);      // epoll_wait()超时，在EventLoop类中回调此函数。

    void OnMessage(spConnection conn,std::string& message); // 处理客户端的请求报文
    
};

