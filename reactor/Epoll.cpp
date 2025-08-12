#include "Epoll.h"

/*
// Epoll类。
class Epoll
{
private:
    static const int MaxEvents=100;                   // epoll_wait()返回事件数组的大小。
    int epollfd_=-1;                                             // epoll句柄，在构造函数中创建。
    epoll_event events_[MaxEvents];                  // 存放poll_wait()返回事件的数组，在构造函数中分配内存。
public:
    Epoll();                                             // 在构造函数中创建了epollfd_。
    ~Epoll();                                          // 在析构函数中关闭epollfd_。

    void addfd(int fd, uint32_t op);                             // 把fd和它需要监视的事件添加到红黑树上。
    std::vector<epoll_event> loop(int timeout=-1);   // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
};
*/

Epoll::Epoll()                                             // 在构造函数中创建了epollfd_。
{
    if((epollfd_ = epoll_create(1)) == -1)
    {
        printf("epoll_create1() failed(%d).\n",errno);
        exit(-1);
    }
}

Epoll::~Epoll()                                          // 在析构函数中关闭epollfd_。
{
    close(epollfd_);
}


void Epoll::updatachannel(Channel *ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->events();
    
    if(ch->inpoll())   //如果channel已经在树上了
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev) == -1)
        {
            printf("epoll_ctl faided(%d).\n", errno);
            exit(-1);
        }
    }
    else             //如果channel不在树上
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev) == -1)
        {
            printf("epoll_ctl faided(%d).\n", errno);
            exit(-1);
        }  
    }

    ch->setinepoll();  //把channel的inepoll_成员设为true
}

void Epoll::removechannel(Channel *ch)
{
    if(ch->inpoll())
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_DEL,ch->fd(),0) == -1)
        {
            printf("epoll_ctl faided(%d).\n", errno);
            exit(-1);
        } 
    }
}

std::vector<Channel*> Epoll::loop(int timeout)   // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
{
    std::vector<Channel*> channels;

    bzero(events_,sizeof(events_));
    int infd = epoll_wait(epollfd_,events_,MaxEvents,timeout);

    //返回失败
    if(infd < 0)
    {
        perror("epoll() failed");
        exit(-1);
    }

    //超时
    if(infd == 0)
    {
        return channels;
    }

    //有事件发生
    for(int i = 0; i < infd; i++)
    {
        //evs.push_back(events_[i]);
        Channel *ch = (Channel*)events_[i].data.ptr;
        ch->setrevents(events_[i].events);
        channels.push_back(ch);
    }

    return channels;
}