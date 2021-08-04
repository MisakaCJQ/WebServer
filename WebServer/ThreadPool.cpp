//
// Created by cjq on 7/27/21.
//
#include <iostream>
#include <unordered_map>
#include <memory>
#include <utility>
#include <cerrno>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ThreadPool.h"
#include "HttpProcessor.h"
#include "Socket.h"
#include "Epoll.h"
using namespace std;

/*
 * 线程池构造函数
 * 主要负责给每个线程创建eventfd，并创建线程
 */
ThreadPool::ThreadPool(int _thread_num)
    :thread_num(_thread_num),taskCount(0),connfds(_thread_num)
{
    if(thread_num<=0)
    {
        cout<<"Wrong create thread pool.Thread_num should be at least 1!"<<endl;
        std::abort();
    }

    //为每个工作线程创建eventfd
    for(int i=0;i<thread_num;i++)
    {
        int efd= eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
        if(efd<0)
        {
            cout<<"Wrong create eventfd!"<<endl;
            std::abort();
        }
        eventfds.push_back(efd);
    }

    //创建工作线程
    for(int i=0;i<thread_num;i++)
    {
        threads.emplace_back(&ThreadPool::eventLoop,this,i,eventfds[i]);
        threads[i].detach();
    }
}

/*
 * 向线程池里添加需要处理的连接
 * 使用轮转算法通过eventfd分配给工作线程
 * 分配成功返回1,失败返回-1
 */
int ThreadPool::addNewConnectionEvent(int connfd)
{
    if(connfd<0)
        return -1;

    if(taskCount>=thread_num)
        taskCount=0;

    uint64_t data=1;
    lock_guard<mutex> guard(newConnectionMutex);//先获取锁再唤醒

    int ret;
    if((ret=write(eventfds[taskCount],&data,sizeof(uint64_t)))<0)
    {
        cout<<ret;
        if(errno!=EAGAIN && errno!=EWOULDBLOCK)
            return -1;
    }

    connfds[taskCount++].push_back(connfd);//将连接socket放入共享变量
    return 1;
}

/*
 * 事件循环
 */
[[noreturn]] void ThreadPool::eventLoop(int num, int efd)
{
    Epoll myEpoll(MAX_CONNS,8);

    //在epollfd上注册可读事件
    //myEpoll.addRecvEvent(make_shared<Socket>(efd));
    myEpoll.setEventfd(make_shared<Socket>(efd));

    unordered_map<int,pair<int,string>> sendingMap;//用于存放等待发送的报文,key为描述符，pair中int 0代表短连接，1代表持久连接.
    //cout<<"thread"<<num<<"eventLoop start!"<<endl;
    while(true)
    {
        vector<myEvent> myEvents;
        myEpoll.EpollWait(myEvents);
        for(const myEvent& event:myEvents)
        {
            if(event.type==NEW_CONN)//eventfd事件
            {
                handleNewConnectionEvent(myEpoll,event.socketPtr,num);
            }
            else if(event.type==RECVABLE)//读就绪事件
            {
                handleRecvEvent(myEpoll,sendingMap,event.socketPtr);
            }
            else if(event.type==SENDABLE)//写就绪事件
            {
                handleSendEvent(myEpoll,sendingMap,event.socketPtr);
            }
            else if(event.type==TIMER)//定时器事件
            {
                handleTimerEvent(myEpoll,event.socketPtr);
            }

        }
    }
}

/*
 * 处理eventfd发来的新连接事件
 * socketPtr为eventfd Socket的指针
 * num为当前所在线程的编号
 */
void ThreadPool::handleNewConnectionEvent(Epoll &myEpoll, const shared_ptr<Socket>& eventSocketPtr, int num)
{
    lock_guard<mutex> guard(newConnectionMutex);

    eventSocketPtr->readUint64();//将eventfd中的计数器重置
    for(const int connfd:connfds[num])
        myEpoll.addRecvEvent(make_shared<Socket>(connfd));
    connfds[num].clear();//清空对应的保存fd的数组

    //由于EPOLLONESHOT，需要重新给eventfd注册读取事件
    myEpoll.resetEventfdOneshot();
}

/*
 * 处理读就绪事件
 * sendingMap为存储待发送消息的哈希表
 * socketPtr为eventfd Socket的指针
 */
void ThreadPool::handleRecvEvent(Epoll &myEpoll, std::unordered_map<int, std::pair<int, std::string>> &sendingMap,
                                 const shared_ptr<Socket> &socketPtr)
{
    //获取HTTP报文
    string message;
    if(socketPtr->getMessage(message)<=0)//读取错误或客户端关闭了连接
    {
        myEpoll.deleteEvent(socketPtr);//从epoll中取消注册
        socketPtr->shutdown();//强制断开连接
        if(sendingMap.count(socketPtr->getFD())!=0)//如果待发消息哈希表中有没来得及发的消息，就删掉
            sendingMap.erase(socketPtr->getFD());
        return;
    }

    //处理HTTP请求报文
    string response;
    int flag=HttpProcessor::handleMethod(message,response);

    //将HTTP响应报文保存起来，并在epoll注册写就绪事件
    sendingMap[socketPtr->getFD()]= make_pair(flag,response);
    myEpoll.resetSendOneshot(socketPtr);
}

/*
 * 处理写就绪事件
 * sendingMap为存储待发送消息的哈希表
 * socketPtr为eventfd Socket的指针
 */
void ThreadPool::handleSendEvent(Epoll &myEpoll, std::unordered_map<int, std::pair<int, std::string>> &sendingMap,
                                 const shared_ptr<Socket> &socketPtr)
{
    if(sendingMap.count(socketPtr->getFD())==0)//没有待写信息,则不处理这一写就绪事件，直接跳过
        return;

    //从哈希表中获取待发送的消息
    int flag=sendingMap[socketPtr->getFD()].first;
    string response=sendingMap[socketPtr->getFD()].second;

    sendingMap.erase(socketPtr->getFD());//将这一条待写信息从哈希表中删除

    ssize_t byteSend=socketPtr->sendMessage(response);//尝试发送
    if(byteSend<0)
    {
        if(errno==EAGAIN || errno==EWOULDBLOCK)//缓冲区满了，发不出去
        {
            myEpoll.resetSendOneshot(socketPtr);//重设EPOLLONESHOT,等待下次写就绪事件
        }
        else//发生其他错误,强制关闭连接
        {
            myEpoll.deleteEvent(socketPtr);
            socketPtr->shutdown();
            cout<<"wrong send message!"<<endl;
        }
    }
    else
    {
        if(byteSend==response.size())//发完了
        {
            if(flag==0)
            {//短连接则删除事件并断开连接
                myEpoll.deleteEvent(socketPtr);
                socketPtr->shutdown();
            }
            else
                myEpoll.resetRecvOneshot(socketPtr);//长连接则重设读就绪EPOLLONESHOT事件
        }
        else//没发完
        {
            sendingMap[socketPtr->getFD()]= make_pair(flag,response.substr(byteSend));//将剩余的消息放入sendingMap
            myEpoll.resetSendOneshot(socketPtr);//重设EPOLLONESHOT事件,等待下次写就绪事件
        }
    }
}

/*
 * 处理定时器事件
 * 旋转时间轮并尝试关闭定时器
 */
void ThreadPool::handleTimerEvent(Epoll &myEpoll, const shared_ptr<Socket> &socketPtr)
{
    socketPtr->readUint64();
    myEpoll.rotateTimingWhell();
    myEpoll.tryStopTimer();
}




