#include "BankServer.h"

BankServer::BankServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum,int sep)
          :tcpserver_(ip,port,subthreadnum,sep),threadpool_(workthreadnum,"WORK")
{
    // 以下代码不是必须的，业务关心什么事件，就指定相应的回调函数。
    tcpserver_.setnewconnectioncb(std::bind(&BankServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&BankServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&BankServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(std::bind(&BankServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(std::bind(&BankServer::HandleSendComplete, this, std::placeholders::_1));
    // tcpserver_.settimeoutcb(std::bind(&BankServer::HandleTimeOut, this, std::placeholders::_1));
    tcpserver_.setrermoveconnectioncb(std::bind(&BankServer::HandleRemove,this,std::placeholders::_1));
}

BankServer::~BankServer()
{

}

// 启动服务。
void BankServer::Start()                
{
    tcpserver_.start();
}

void BankServer::Stop()
{
    //停止工作线程
    threadpool_.Stop();
    printf("工作线程已停止\n");

    //停止IO线程（事件循环）
    tcpserver_.Stop();
}

// 处理新客户端连接请求，在TcpServer类中回调此函数。
void BankServer::HandleNewConnection(spConnection conn)    
{
    // 新客户端连上来时，保存用户信息到状态机中。
    spUserInfo userinfo(new UserInfo(conn->fd(),conn->ip()));
    {
        std::lock_guard<std::mutex> gd(mutex_);
        usermap_[conn->fd()] = userinfo;
    }
    printf("%s新建连接(ip=%s).\n",Timestamp::now().tostring().c_str(),conn->ip().c_str());
}

// 关闭客户端的连接，在TcpServer类中回调此函数。 
void BankServer::HandleClose(spConnection conn)  
{
    //关闭客户端连接时，从状态机中删除连接的信息
    printf ("%s 连接断开(ip=%s).\n",Timestamp::now().tostring().c_str(),conn->ip().c_str());
    {
        std::lock_guard<std::mutex> gd(mutex_);
        usermap_.erase(conn->fd());
    } 
}

// 客户端的连接错误，在TcpServer类中回调此函数。
void BankServer::HandleError(spConnection conn)  
{
    HandleClose(conn);
}

// 处理客户端的请求报文，在TcpServer类中回调此函数。
void BankServer::HandleMessage(spConnection conn,std::string &message)     
{
    if(threadpool_.size() == 0)
    {
        // 如果没有工作线程，表示在IO线程中计算。
        OnMessage(conn,message);
    }
    else
    {
        // 把业务添加到线程池的任务队列中，交给工作线程去处理业务。
        threadpool_.addtask(std::bind(&BankServer::OnMessage,this,conn,message));
    }
}

bool getxmlbuffer(const std::string &xmlbuffer,const std::string &fieldname,std::string &value,const int ilen=0)
{
    std::string start="<"+fieldname+">";
    std::string end="</"+fieldname+">";

    int startp=xmlbuffer.find(start);
    if(startp==std::string::npos) return false; //在xml中查找数据项开始的标签的位置

    int endp=xmlbuffer.find(end);
    if(endp==std::string::npos) return false;   //在xml中查找数据项结束的标签的位置

    //从xml中截取数据项的内容
    int itmplen=endp-startp-start.length();
    if((ilen>0) && (ilen<itmplen)) itmplen=ilen;
    value=xmlbuffer.substr(startp+start.length(),itmplen);

    return true;
}

// 处理客户端的请求报文
void BankServer::OnMessage(spConnection conn,std::string& message)
{
    spUserInfo userinfo = usermap_[conn->fd()];  // 从状态机中获取客户端信息。
    std::string bizcode;         //业务代码
    std::string replaymessage;   //回应报文
    getxmlbuffer(message,"bizcode",bizcode); //从请求报文中解析业务代码

    if(bizcode == "00101") //登陆业务
    {
        std::string username,password;
        getxmlbuffer(message,"username",username);
        getxmlbuffer(message,"password",password);
        if(username == "jhc" && password == "123456")  //实际上密码用户名一般在数据库或Redis中
        {
            replaymessage = "<bizcode>00102</bizcode><retcode>0</retcode><message>ok</message>";
            userinfo->setLogin(true);
        }
        else
        {
            replaymessage = "<bizcode>00102</bizcode><retcode>-1</retcode><message>用户名或密码不正确</message>";
        }
    }
    else if(bizcode == "00201") //查询余额业务
    {
        if(userinfo->Login() == true)
        {
            replaymessage = "<bizcode>00202</bizcode><retcode>0</retcode><message>8888</message>";
        }
        else
        {
            replaymessage = "<bizcode>00202</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }
    else if(bizcode == "00901") //注销业务
    {
        if(userinfo->Login() == true)
        {
            printf("注销成功\n");
            replaymessage = "<bizcode>00902</bizcode><retcode>0</retcode><message>ok</message>";
            userinfo->setLogin(false);
        }
        else
        {
            replaymessage = "<bizcode>00902</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }
    
    else if(bizcode == "00001") //心跳
    {
        if(userinfo->Login() == true)
        {
            replaymessage = "<bizcode>00002</bizcode><retcode>0</retcode><message>ok</message>";
        }
        else
        {
            replaymessage = "<bizcode>00002</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }

    conn->send(replaymessage.data(),replaymessage.size());   // 把数据发送出去。
}

// 数据发送完成后，在TcpServer类中回调此函数。
void BankServer::HandleSendComplete(spConnection conn)     
{
    //std::cout << "Message send complete." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}

/*
// epoll_wait()超时，在TcpServer类中回调此函数。
void BankServer::HandleTimeOut(EventLoop *loop)         
{
    std::cout << "BankServer timeout." << std::endl;

    // 根据业务的需求，在这里可以增加其它的代码。
}
*/

void BankServer::HandleRemove(int fd)
{
    printf("fd(%d) 已超时\n",fd);

    std::lock_guard<std::mutex> gd(mutex_);
    usermap_.erase(fd);
}