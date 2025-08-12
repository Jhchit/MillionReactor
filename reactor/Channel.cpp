#include "Channel.h"


Channel::Channel(EventLoop *loop,int fd):loop_(loop),fd_(fd)      // 构造函数。
{

}

Channel::~Channel()                           // 析构函数。 
{

}

int Channel::fd()                                            // 返回fd_成员。
{
    return fd_;
}

void Channel::useet()                                    // 采用边缘触发。
{
    events_  |= EPOLLET;
}

void Channel::enablereading()                     // 让epoll_wait()监视fd_的读事件。
{
    events_ |= EPOLLIN;
    loop_->updatachannel(this);
}

void Channel::disablereading()
{
    events_ &= ~EPOLLIN;
    loop_->updatachannel(this);
}

void Channel::enablewriting()
{
    events_ |= EPOLLOUT;
    loop_->updatachannel(this);
}

void Channel::disablewriting()
{
    events_ &= ~EPOLLOUT;
    loop_->updatachannel(this);
}

// 取消全部的事件。
void Channel::disableall()
{
    events_ = 0;
    loop_->updatachannel(this);
}   

// 从事件循环中删除Channel。
void Channel::remove()
{
    loop_->removechannel(this);
}

void Channel::setinepoll()                            // 把inepoll_成员的值设置为true。
{
    inepoll_ = true;
}

void Channel::setrevents(uint32_t ev)         // 设置revents_成员的值为参数ev。
{
    revents_ = ev;
}

bool Channel::inpoll()                                  // 返回inepoll_成员。
{
    return inepoll_;
}

uint32_t Channel::events()                           // 返回events_成员。
{
    return events_;
}

uint32_t Channel::revents()                          // 返回revents_成员。
{
    return revents_;
}

void Channel::handleevents()                                 //事件处理函数，epoll_wait()返回时，执行它
{
    if(revents_ & EPOLLRDHUP)  //对方断开连接
    {
        closecallback_();
    }
    else if(revents_ & (EPOLLIN|EPOLLPRI))  //接收缓冲区有数据可读
    {
        readcallback_();
    }
    else if(revents_ & EPOLLOUT)  //写事件
    {
        writecallback_();
    }
    else  //其他（其他事件都视为错误））
    {
        errorcallback_();
    }
}


void Channel::setreadcallback(std::function<void()> fn)
{
    readcallback_ = fn;
}

void Channel::setclosecallback(std::function<void()> fn)   // 设置关闭fd_的回调函数。
{
    closecallback_ = fn;
}

void Channel::seterrorcallback(std::function<void()> fn)   // 设置fd_发生了错误的回调函数。
{
    errorcallback_ = fn;
}

void Channel::setwritecallback(std::function<void()> fn)
{
    writecallback_ = fn;
}