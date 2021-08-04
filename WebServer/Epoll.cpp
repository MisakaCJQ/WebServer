//
// Created by cjq on 7/29/21.
//
#include <iostream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "Epoll.h"

using namespace std;
/*
 * 构造函数
 * 主要是获取epollfd和timerfd
 * 并将timerfd加入内核事件表
 */
Epoll::Epoll(int maxConns,int expiredTime)
    :epfdSocket(epoll_create(maxConns)), timingWheel(expiredTime),isTimerStart(false),
    timerfdSocketPtr(make_shared<Socket>(timerfd_create(CLOCK_REALTIME,TFD_CLOEXEC|TFD_NONBLOCK)))
{
    if(epfdSocket.getFD() < 0 ||timerfdSocketPtr->getFD()<0)
    {
        cout<<"create epollfd or timerfd fail!"<<endl;
        std::abort();
    }

    //将timerfd加入内核事件表
    epoll_event event{};
    event.data.fd= timerfdSocketPtr->getFD();
    event.events=EPOLLIN | EPOLLET |EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_ADD, timerfdSocketPtr->getFD(), &event);
    timerfdSocketPtr->setNonBlocking();//将socket设置为非阻塞式
}

/*
 * 设置eventfd
 * 对其不使用定时器管理
 */
void Epoll::setEventfd(const shared_ptr<Socket> &socketPtr)
{
    eventfdSocketPtr=socketPtr;//将管理eventfd的Socket保存起来

    epoll_event event{};
    event.data.fd= socketPtr->getFD();
    event.events=EPOLLIN | EPOLLET |EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_ADD, socketPtr->getFD(), &event);
    socketPtr->setNonBlocking();//将socket设置为非阻塞式
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
    Sockets[socketPtr->getFD()]=weak_ptr<Socket>(socketPtr);//将这一fd以weak-ptr保存起来
    timingWheel.addSocket(socketPtr);//将这一Socket加入时间轮
    tryStartTimer();
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
    Sockets[socketPtr->getFD()]=weak_ptr<Socket>(socketPtr);//将这一fd以weak-ptr保存起来
    timingWheel.addSocket(socketPtr);//将这一Socket加入时间轮
    tryStartTimer();
}

/*
 * 重新设置eventfd的ONESHOT事件
 * 对其不使用定时器管理
 */
void Epoll::resetEventfdOneshot()
{
    epoll_event event{};
    event.data.fd= eventfdSocketPtr->getFD();
    event.events=EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(), EPOLL_CTL_MOD, eventfdSocketPtr->getFD(), &event);
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
    timingWheel.addSocket(socketPtr);//将这一Socket加入时间轮
    tryStartTimer();
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
    timingWheel.addSocket(socketPtr);//将这一Socket加入时间轮
    tryStartTimer();
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
    {//遍历events,将事件类型(RECVABLE,SENDABLE,NEW_CONN,TIMER)和Socket指针封装到自定义结构体中
        if(events[i].data.fd==eventfdSocketPtr->getFD())
            myEvents.push_back(myEvent{eventfdSocketPtr,NEW_CONN});
        else if(events[i].data.fd==timerfdSocketPtr->getFD())
            myEvents.push_back(myEvent{timerfdSocketPtr,TIMER});
        else
        {
            if(events[i].events & EPOLLIN)
                myEvents.push_back(myEvent{Sockets[events[i].data.fd].lock(),RECVABLE});
            else if(events[i].events & EPOLLOUT)
                myEvents.push_back(myEvent{Sockets[events[i].data.fd].lock(),SENDABLE});
            else
                cout<<"wrong event in epoll wait!"<<endl;
        }
    }

    return ret;
}

/*
 * 对时间轮进行旋转操作
 */
void Epoll::rotateTimingWhell()
{
    timingWheel.rotate();
}

/*
 * 尝试关闭timerfd定时器
 * 如果定时器已关闭或时间轮中还有Socket则不进行任何操作
 */
void Epoll::tryStopTimer()
{
    if(!isTimerStart)//定时器已关闭，不操作
        return;

    if(timingWheel.getaliveSocketNum()==0)
    {
        itimerspec new_value{};

        //设定定时器启动时间为0
        new_value.it_value.tv_sec=0;
        new_value.it_value.tv_nsec=0;

        //设定定时器触发周期为0
        new_value.it_interval.tv_sec=0;
        new_value.it_interval.tv_nsec=0;
        timerfd_settime(timerfdSocketPtr->getFD(),0, &new_value, nullptr);
        isTimerStart=false;
    }
    else
    {//时间轮仍有从存活Socket，则不不关闭定时器，重设ONESHOT事件
        resetTimerfdOneshot();
    }
}

/*
 * 尝试启动timerfd定时器
 * 如果已经启动则不进行任何操作
 */
void Epoll::tryStartTimer()
{
    //cout<<"try start timer"<<endl;
    if(isTimerStart)//定时器已开启，不操作
        return;

    itimerspec new_value{};

    //设定定时器启动时间，这里为相对时间，1s
    new_value.it_value.tv_sec=1;
    new_value.it_value.tv_nsec=0;

    //设定定时器触发周期为1秒
    new_value.it_interval.tv_sec=1;
    new_value.it_interval.tv_nsec=0;

    if(timerfd_settime(timerfdSocketPtr->getFD(),0,&new_value, nullptr)<0)
    {
        cout<<"start timer error!"<<endl;
        std::abort();
    }
    resetTimerfdOneshot();
    isTimerStart=true;
}

/*
 * 重新设置timerfd的ONESHOT事件
 * 对其不使用定时器管理
 */
void Epoll::resetTimerfdOneshot()
{
    epoll_event event{};
    event.data.fd=timerfdSocketPtr->getFD();
    event.events=EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl(epfdSocket.getFD(),EPOLL_CTL_MOD,timerfdSocketPtr->getFD(),&event);
}

