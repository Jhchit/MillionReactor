#include "Connection.h"

Connection::Connection(EventLoop *loop,std::unique_ptr<Socket>clientsock,int sep)
           :clientsock_(std::move(clientsock)),loop_(loop),disconnect_(false)
           ,clientchannel_(new Channel(loop_,clientsock_->fd())),inputbuffer_(sep)
{
    // 为新客户端连接准备读事件，并添加到树上。
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
    clientchannel_->useet();            // 客户端连上来的fd采用边缘触发。
    clientchannel_->enablereading();
}

Connection::~Connection()
{
    //printf("Connection对象已析构。\n");
}

int Connection::fd() const
{
    return clientsock_->fd();
} 

std::string Connection::ip() const
{
    return clientsock_->ip();
}

uint16_t Connection::port() const
{
    return clientsock_->port();
}

void Connection::onmessage()
{
    char buffer[1024];
    while(true)
    {
        memset(buffer,0,sizeof(buffer));
        ssize_t nread = read(fd(),buffer,sizeof(buffer));
        if(nread > 0) //成功读取数据
        {
            //把数据传回去
            inputbuffer_.append(buffer,nread);
        }
        else if(nread == -1 && errno == EINTR) //读数据时被信号中断，继续读取
        {
            continue;
        }
        else if(nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) //数据已读玩
        {
            std::string message;
            
            while(true)
            {
                if(inputbuffer_.pickmessage(message) == false) break;
                
                //printf("message (eventfd=%d):%s\n",fd(),message.c_str());
                lasttime_ = Timestamp::now();

                onmessagecallback_(shared_from_this(),message);
            }
            break;
        }
        else if(nread == 0)    //客户端断开
        {
            closecallback();
            break;
        }
    }

}

// TCP连接关闭（断开）的回调函数，供Channel回调。
void Connection::closecallback()
{
    disconnect_ = true;
    clientchannel_->remove();
    closecallback_(shared_from_this());
}                

 // TCP连接错误的回调函数，供Channel回调。
void Connection::errorcallback()
{
    disconnect_ = true;
    clientchannel_->remove();
    errorcallback_(shared_from_this());
}                

void Connection::writecallback()                   // 处理写事件的回调函数，Channel回调
{
    int writen  = ::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0); //尝试把putbuffer中的数据全部发送出去
    if(writen > 0) outputbuffer_.erase(0,writen);                           //清除已经发生的字节数

    //如果outbuffer已经空了，停止关注写事件
    if(outputbuffer_.size() == 0) 
    {
        clientchannel_->disablewriting();
        sendcompletecallback_(shared_from_this());
    }

}

void Connection::setclosecallback(std::function<void(spConnection)> fn)
{
    closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(spConnection)> fn)
{
    errorcallback_ = fn;
}

void Connection::setonmessagecallback( std::function<void(spConnection,std::string&)> fn)
{
    onmessagecallback_ = fn;
}

void Connection::send(const char *data,size_t size)
{
    if(disconnect_ == true)
    {
        printf("客户端已断开，send()直接返回.\n");return;
    }
    
    send_buffer_.assign(data,data + size);
    if(loop_->isinloopthread()) // 判断当前线程是否为事件循环线程（IO线程）。
    {
        // 如果当前线程是IO线程，直接调用sendinloop()发送数据。
        //printf("send() 在事件循环的线程中。\n");
        sendinloop(send_buffer_.data(),send_buffer_.size());
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程去执行。
        //printf("send() 不在事件循环的线程中。\n");
        loop_->queueinloop(std::bind(&Connection::sendinloop,this,send_buffer_.data(),send_buffer_.size()));
    }

}

 // 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
 void Connection::sendinloop(const char *data,size_t size)
 {
    outputbuffer_.appendwithsep(data,size);
    clientchannel_->enablewriting();
 }

 void Connection::setsendcompletecallback( std::function<void(spConnection)> fn)
 {
    sendcompletecallback_ = fn;
 }

// 判断连接是否超时（空闲太久）。
bool Connection::timeout(time_t now,int val)
{
    return now - lasttime_.toint() > val ;
}