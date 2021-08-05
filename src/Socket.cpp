//
// Created by cjq on 7/25/21.
//
#include "Socket.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>

using namespace std;

Socket::Socket():isAlive(true)
{
    fd=socket(AF_INET,SOCK_STREAM,0);
}

Socket::~Socket()
{
    shutdown();
}
/*
 * 用于将socket设置地址重用参数
 * 设置失败终止程序
 */
void Socket::setReuseAddr() const
{
    /*-------------设定监听地址重用,避免由于TCP的TIME-WAIT导致绑定地址失败-------------*/
    int optval=1;
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval))<0)
    {//地址重用设定失败则直接终止程序
        cout<<"wrong set socket REUSEADDR!"<<endl;
        std::abort();
    }
}

/*
 * 将socket设定为非阻塞
 */
void Socket::setNonBlocking() const
{
    int flags;
    if((flags=fcntl(fd,F_GETFL,0))<0)
    {
        cout<<"wrong set socket nonblocking!"<<endl;
        std::abort();
    }
    flags |= O_NONBLOCK;
    if(fcntl(fd,F_SETFL,flags)<0)
    {
        cout<<"wrong set socket nonblocking!"<<endl;
        std::abort();
    }
}

/*
 * 将socket绑定到特定的地址和端口上
 * 绑定失败则终止程序
 */
void Socket::Bind(const string &ip, int port) const
{
    /*----设定地址结构----*/
    sockaddr_in servaddr{};
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;//设定ipv4协议
    inet_pton(AF_INET,ip.c_str(),&servaddr.sin_addr);//设定地址
    servaddr.sin_port= htons(port);//设定端口

    /*----bind-----*/
    if (bind(fd,(sockaddr*)&servaddr,sizeof(servaddr))<0)
    {//bind失败则直接终止程序
        cout<<"wrong bind in"<<ip<<':'<<port<<'!'<<endl;
        std::abort();
    }
}

/*
 * 用于让socket开始监听
 * 失败则终止程序
 */
void Socket::Listen(int maxConnNum) const
{
    if(listen(fd,maxConnNum)<0)
    {
        cout<<"wrong listen!"<<endl;
        std::abort();
    }
    cout<<"start listening...."<<endl;
}

/*
 * 用于从监听socket获取新的连接socket
 */
int Socket::Accept() const
{

    sockaddr_in cliaddr{};
    socklen_t clilen=sizeof(cliaddr);

    return accept(fd,(sockaddr*)&cliaddr,&clilen);
}

/*
 * 获取fd关键字
 */
int Socket::getFD() const
{
    return fd;
}

/*
 * 获取HTTP请求报文
 * 可在非阻塞式socket使用
 */
int Socket::getMessage(string &data) {
    //用于非阻塞IO的读取
    vector<char> buf(MAX_BUF_SIZE);//定义缓冲区
    ssize_t byterecv=0;
    //循环读取数据
    while(true)
    {
        byterecv= recv(fd,&buf[0],MAX_BUF_SIZE,0);
        if(byterecv<0)
        {
            if(errno==EAGAIN || errno==EWOULDBLOCK)//读完了数据
                return 1;
            else
            {
                cout<<"wrong recv!"<<endl;
                return -1;
            }
        }
        else if(byterecv==0)//客户端关闭连接
            return 0;
        else
            data.append(buf.cbegin(),buf.cbegin()+byterecv);
    }
}

/*
 * 向客户端发送HTTP应答报文
 */
ssize_t Socket::sendMessage(const string &data) {
    return send(fd,data.c_str(),data.size(),0);
}

/*
 * 读取uint64
 * 主要用于重设eventfd,timerfd
 */
void Socket::readUint64()
{
    uint64_t rdata;
    if(read(fd,&rdata,sizeof(uint64_t))<0)
        if(errno!=EAGAIN && errno!=EWOULDBLOCK)
            cout<<"wrong recv eventfd"<<endl;
}

/*
 * 强制关闭fd
 */
void Socket::shutdown()
{
    if(isAlive)
    {
        isAlive= false;
        if(recvingBufferPtr!= nullptr)
        {//如果待收缓存中仍有未收完的报文，就删除
            if(recvingBufferPtr->count(fd)!=0)
                recvingBufferPtr->erase(fd);
        }
        close(fd);
    }
}



