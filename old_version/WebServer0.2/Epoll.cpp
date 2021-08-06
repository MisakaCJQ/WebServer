//
// Created by cjq on 7/29/21.
//
#include <iostream>
#include <sys/epoll.h>
#include "Epoll.h"

using namespace std;
/*
 * 构造函数
 * 主要是获取epollfd
 */
Epoll::Epoll(int maxConns)
    :epfdSocket(epoll_create(maxConns))
{
    if(epfdSocket.getFD() < 0)
    {
        cout<<"create epoll fd fail!"<<endl;
        std::abort();
    }
}

/*
 * 向epoll内核事件表中添加描述符
 * 注册可读事件，使用ET+EPOLLONESHOT模式
 */
void Epoll::addRecvEvent(const std::shared_ptr<Socket>& socketPtr)
{
    epoll_event event{};
    event.data.fd= socketPtr->getFD();
    event.events=EPOLLIN | EPOLLET |EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_ADD, socketPtr->getFD(), &event);
    socketPtr->setNonBlocking();//将socket设置为非阻塞式
    Sockets[socketPtr->getFD()]=socketPtr;//将这一fd保存起来
}

/*
 * 向epoll内核事件表中添加描述符
 * 注册可写事件，使用ET+EPOLLONESHOT模式
 */
void Epoll::addSendEvent(const shared_ptr<Socket> &socketPtr)
{
    epoll_event event{};
    event.data.fd= socketPtr->getFD();
    event.events=EPOLLOUT | EPOLLET |EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_ADD, socketPtr->getFD(), &event);
    socketPtr->setNonBlocking();//将socket设置为非阻塞式
    Sockets[socketPtr->getFD()]=socketPtr;//将这一fd保存起来
}

/*
 * 重设可读ET+EPLLONESHOT模式
 */
void Epoll::resetRecvOneshot(const shared_ptr<Socket> &socketPtr)
{
    epoll_event event{};
    event.data.fd= socketPtr->getFD();
    event.events=EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_MOD, socketPtr->getFD(), &event);
}

/*
 * 重设可写ET+EPLLONESHOT模式
 */
void Epoll::resetSendOneshot(const shared_ptr<Socket> &socketPtr)
{
    epoll_event event{};
    event.data.fd= socketPtr->getFD();
    event.events=EPOLLOUT | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_MOD, socketPtr->getFD(), &event);
}

/*
 * 从epoll内核事件表中删除描述符注册的事件
 */
void Epoll::deleteEvent(const shared_ptr<Socket> &socketPtr)
{
    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_DEL, socketPtr->getFD(), nullptr);
    if(Sockets.count(socketPtr->getFD()) != 0)//从哈希表里删除
        Sockets.erase(socketPtr->getFD());
}

/*
 * 对epoll_wait进行封装
 * 通过参数myEvents将就绪事件传出
 * 返回值为就绪事件数量
 */
int Epoll::EpollWait(vector<myEvent> &myEvents)
{
    myEvents.clear();
    epoll_event events[MAX_EVENT_NUM];

    //cout<<"start epoll wait in epfd "<<epfdSocket.getFD()<<endl;

    int ret=epoll_wait(epfdSocket.getFD(), events, MAX_EVENT_NUM, -1);
    //cout<<"get an epoll"<<endl;
    //FIXME 没有处理events有错误的情况
    for(int i=0;i<ret;i++)
    {//遍历events,将事件类型(RECVABLE,SENDABLE)和Socket指针封装到自定义结构体中
        if(events[i].events & EPOLLIN)
            myEvents.push_back(myEvent{Sockets[events[i].data.fd],RECVABLE});
        else if(events[i].events & EPOLLOUT)
            myEvents.push_back(myEvent{Sockets[events[i].data.fd],SENDABLE});
        else
            cout<<"wrong event in epoll wait!"<<endl;
    }

    return ret;
}

