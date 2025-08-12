#pragma once
#include "Acceptor.h"
#include "Connection.h"
#include <map>
#include "ThreadPool.h"
#include <mutex>

//TCP网络服务类
class TcpServer
{
private:
    std::unique_ptr<EventLoop>mainloop_;                  // 主事件循环
    std::vector<std::unique_ptr<EventLoop>> subloops_;     // 存放从事件循环
    Acceptor acceptor_;                    // 一个TcpServer只有一个Acceptor对象。
    int threadnum_;                        //线程池的大小，同时也是从事件循环数
    ThreadPool threadpool_;                // 线程池
    std::mutex mutex_;                     // 保护conns_的互斥锁。
    std::map<int,spConnection> conns_;     // 一个TcpServer有多个Connection对象，存放在map容器中。
    std::function<void(spConnection)> newconnectioncb_;          // 回调EchoServer::HandleNewConnection()。
    std::function<void(spConnection)> closeconnectioncb_;        // 回调EchoServer::HandleClose()。
    std::function<void(spConnection)> errorconnectioncb_;         // 回调EchoServer::HandleError()。
    std::function<void(spConnection,std::string &message)> onmessagecb_;        // 回调EchoServer::HandleMessage()。
    std::function<void(spConnection)> sendcompletecb_;            // 回调EchoServer::HandleSendComplete()。
    std::function<void(EventLoop*)>  timeoutcb_;                  // 回调EchoServer::HandleTimeOut()。
    std::function<void(int)>  rermoveconnectioncb_;               //回调上层业务类的BankServer::HandleRemove();
    int sep_;
    
public:
    TcpServer(const std::string &ip,const uint16_t port,int threadnum = 3,int sep = 0);
    ~TcpServer();

    void start();
    void Stop();
    
    void newconnection(std::unique_ptr<Socket>clientsock);
    void closeconnection(spConnection conn);  // 关闭客户端的连接，在Connection类中回调此函数。 
    void errorconnection(spConnection conn);  // 客户端的连接错误，在Connection类中回调此函数。
    void onmessage(spConnection conn,std::string &message);     // 处理客户端的请求报文，在Connection类中回调此函数。
    void sendcomplete(spConnection conn);     // 数据发送完成后，在Connection类中回调此函数。
    void epolltimeout(EventLoop *loop);      // epoll_wait()超时，在EventLoop类中回调此函数。

    void setnewconnectioncb(std::function<void(spConnection)> fn);
    void setcloseconnectioncb(std::function<void(spConnection)> fn);
    void seterrorconnectioncb(std::function<void(spConnection)> fn);
    void setonmessagecb(std::function<void(spConnection,std::string &message)> fn);
    void setsendcompletecb(std::function<void(spConnection)> fn);
    void settimeoutcb(std::function<void(EventLoop*)> fn);

    void removeconn(int fd);      // 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数。
    void setrermoveconnectioncb(std::function<void(int)> fn);
};
