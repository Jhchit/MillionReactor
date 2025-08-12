#include "InetAddress.h"

/*
#pragma once

#include <arpa/inet.h>
#include <string>
#include <netinet/in.h>

class InetAddress
{
private:
    sockaddr_in addr_;
public:
    InetAddress(const std::string &ip,uint16_t port);
    InetAddress(const sockaddr_in addr);
    ~InetAddress();

    const char *ip() const;
    uint16_t port() const;
    const sockaddr* addr() const;
};
*/
InetAddress::InetAddress()
{

}

InetAddress::InetAddress(const std::string &ip,uint16_t port)
{
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);     //端口
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());//IP地址 //把字符串格式的ip转为大端序的ip
}

InetAddress::InetAddress(const sockaddr_in addr):addr_(addr)
{

}

InetAddress::~InetAddress()
{}

const char *InetAddress::ip() const
{
    return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::port() const
{
    return ntohs(addr_.sin_port);
}

const sockaddr* InetAddress::addr() const
{
    return (sockaddr*)&addr_;
}

void InetAddress::setaddr(sockaddr_in clientaddr)
{
    addr_ = clientaddr;
}