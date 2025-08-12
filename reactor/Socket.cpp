#include "Socket.h"

// 创建一个非阻塞的socket。
int createnonblocking()
{
    // 创建服务端用于监听的listenfd。
    int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);
    if (listenfd < 0)
    {
        // perror("socket() failed"); exit(-1);
        printf("%s:%s:%d listen socket create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno); exit(-1);
    }
    return listenfd;
}

Socket::Socket(int fd):fd_(fd)
{

}

Socket::~Socket()
{
    ::close(fd_);
}

int Socket::fd() const
{
    return fd_;
} 

std::string Socket::ip() const
{
    return ip_;
}

uint16_t Socket::port() const
{
    return port_;
}             

void Socket::setreuseaddr(bool on)  // 设置SO_REUSEADDR选项，true-打开，false-关闭。
{
    int opt = on ? 1:0;
    setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt));
}    

void Socket::setreuseport(bool on)       // 设置SO_REUSEPORT选项。
{
    int opt = on ? 1:0;
    setsockopt(fd_,SOL_SOCKET,TCP_NODELAY ,&opt,static_cast<socklen_t>(sizeof opt));
}

void Socket::settcpnodelay(bool on)     // 设置TCP_NODELAY选项。
{
    int opt = on ? 1:0;
    setsockopt(fd_,SOL_SOCKET,SO_REUSEPORT,&opt,static_cast<socklen_t>(sizeof opt));
}

void Socket::setkeepalive(bool on)       // 设置SO_KEEPALIVE选项。
{
    int opt = on ? 1:0;
    setsockopt(fd_,SOL_SOCKET,SO_KEEPALIVE,&opt,static_cast<socklen_t>(sizeof opt));
}

void Socket::bind(const InetAddress& servaddr)
{
    if(::bind(fd_,servaddr.addr(),sizeof(sockaddr)) != 0)
    {
        perror("bind() failed");
        close(fd_);
        exit(-1);
    }

    setipport(servaddr.ip(),servaddr.port());
}

void Socket::listen(int nn)
{
    if(::listen(fd_,nn) != 0)
    {
        perror("listen() failed");
        close(fd_);
        exit(-1);
    }
}

//Socket::accept函数不仅要创建一个clientfd，还要把listenfd的地址信息传递给clientfd
//这里传给了在main函数中创建的空的INetAddress对象clienaddr
int Socket::accept(InetAddress& clientaddr)
{
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_,(sockaddr*)&peeraddr,&len,SOCK_NONBLOCK);

    // 客户端的地址和协议。（上一步中accept4把listenfd的地址信息给与了peeraddr，这一步把peeraddr赋予空的clientaddr
    clientaddr.setaddr(peeraddr);  

    // 注意：以下两行代码有问题，这个问题将会被修复，以后再说，先不要管它。

    return clientfd; 
}

void Socket::setipport(const std::string &ip,uint16_t port)
{
    ip_ = ip;
    port_ = port;
}