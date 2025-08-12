#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum,int sep)
         :threadnum_(threadnum),threadpool_(threadnum_,"IO"),mainloop_(new EventLoop(true))
         ,acceptor_(mainloop_.get(),ip,port),sep_(sep)
{
    // 设置epoll_wait()超时的回调函数
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));

    // 设置处理新客户端连接请求的回调函数。
    acceptor_.setnewconnection(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

    // 创建从事件循环,从事件循环数等于IO线程数。
    for(int ii = 0;ii < threadnum_;ii++)
    {
        subloops_.emplace_back(new EventLoop(false,5,10));
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));// 设置timeout超时的回调函数。
        subloops_[ii]->settimercallback(std::bind(&TcpServer::removeconn,this,std::placeholders::_1));// 设置清理空闲TCP连接的回调函数。
        threadpool_.addtask(std::bind(&EventLoop::run,subloops_[ii].get()));
    }
}

TcpServer::~TcpServer()
{

}

void TcpServer::start()
{
    mainloop_->run();
}

void TcpServer::Stop()
{
    // 停止主事件循环
    mainloop_->Stop();
    printf("主事件循环已停止。\n");
    // 停止从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_[ii]->Stop();
    }
    printf("从事件循环已停止。\n");

    // 停止IO线程。
    threadpool_.Stop();
    printf("IO线程池停止。\n");
}

void TcpServer::newconnection(std::unique_ptr<Socket>clientsock)
{
    spConnection conn(new Connection(subloops_[clientsock->fd()%threadnum_].get(),std::move(clientsock),sep_));
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));
    
    {
        std::lock_guard<std::mutex> gd(mutex_);
        conns_[conn->fd()] = conn;  // 把conn存放map容器中。
    }

    subloops_[conn->fd()%threadnum_]->newconnection(conn);

    if (newconnectioncb_) newconnectioncb_(conn);             // 回调EchoServer::HandleNewConnection()。
}

void TcpServer::closeconnection(spConnection conn)  // 关闭客户端的连接，在Connection类中回调此函数。 
{
    if (closeconnectioncb_) closeconnectioncb_(conn);       // 回调EchoServer::HandleClose()。

    //printf("client (eventfd=%d) disconnected.\n",conn->fd());  
    {
        std::lock_guard<std::mutex> gd(mutex_);
        conns_.erase(conn->fd());  // 从map中删除conn。
    }
}

void TcpServer::errorconnection(spConnection conn)  // 客户端的连接错误，在Connection类中回调此函数。
{
    if (errorconnectioncb_) errorconnectioncb_(conn);     // 回调EchoServer::HandleError()。
    {
        std::lock_guard<std::mutex> gd(mutex_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::onmessage(spConnection conn,std::string &message)
{
    if (onmessagecb_) onmessagecb_(conn,message);     // 回调EchoServer::HandleMessage()。
}

void TcpServer::sendcomplete(spConnection conn)
{
     if (sendcompletecb_) sendcompletecb_(conn);     // 回调EchoServer::HandleSendComplete()。
}

// epoll_wait()超时，在EventLoop类中回调此函数。
void TcpServer::epolltimeout(EventLoop *loop)
{
    if (timeoutcb_)  timeoutcb_(loop);           // 回调EchoServer::HandleTimeOut()。
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn)
{
    newconnectioncb_=fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_=fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_=fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection,std::string &message)> fn)
{
    onmessagecb_=fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_=fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_=fn;
}

void TcpServer::removeconn(int fd)
{
    std::lock_guard<std::mutex> gd(mutex_);
    if (conns_.find(fd) != conns_.end()) 
    {
        conns_.erase(fd); // 从 TcpServer的map中把conns_ 删除
    }

    if(rermoveconnectioncb_) rermoveconnectioncb_(fd);
}

void TcpServer::setrermoveconnectioncb(std::function<void(int)> fn)
{
    rermoveconnectioncb_=fn;
}
