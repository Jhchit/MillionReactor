#include "EchoServer.h"
#include <signal.h>

EchoServer *echoserver;

//信号2和5的处理函数，用来停止服务程序
void Stop(int sig) 
{
    printf("sig = %d\n",sig);
    echoserver->Stop();
    printf("echoserver 已停止.\n");
    delete echoserver;
    printf("delete echoserver\n");
    exit(0);
}

int main(int argc,char *argv[])
{
    if(argc != 3)
    {
        printf("usage: ./tcpepoll ip port\n"); 
        printf("example: ./tcpepoll 127.0.0.1 8080\n"); 
        return 1;
    }
   
    signal(SIGTERM,Stop);//信号15，系统kill或killall命令默认发送的信号
    signal(SIGINT,Stop);//信号2，ctrl+c发送的信号

    echoserver = new EchoServer(argv[1],atoi(argv[2]),3,2,1);
    echoserver->Start();

    return 0;
}