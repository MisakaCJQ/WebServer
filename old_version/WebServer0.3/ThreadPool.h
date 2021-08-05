//
// Created by cjq on 7/27/21.
//
#include <vector>
#include <thread>
#include <mutex>
#include "Socket.h"
#include "Epoll.h"
#ifndef HTTP_SERVER_THREADPOOL_H
#define HTTP_SERVER_THREADPOOL_H


class ThreadPool {
public:
    explicit ThreadPool(int _thread_num);
    int addNewConnectionEvent(int connfd);
private:
    [[noreturn]] void eventLoop(int num,int efd);
    void handleNewConnectionEvent(Epoll &myEpoll, const std::shared_ptr<Socket>& eventSocketPtr, int num);
    void handleRecvEvent(Epoll &myEpoll,std::unordered_map<int,std::pair<int,std::string>> &sendingMap,const std::shared_ptr<Socket>& socketPtr);
    void handleSendEvent(Epoll &myEpoll,std::unordered_map<int,std::pair<int,std::string>> &sendingMap,const std::shared_ptr<Socket>& socketPtr);
    void handleTimerEvent(Epoll &myEpoll,const std::shared_ptr<Socket>& socketPtr);
private:
    int thread_num;//线程数
    int taskCount;//用于连接分配的轮转算法
    std::vector<std::thread> threads;//保存工作线程对象
    std::vector<int> eventfds;
    std::vector<std::vector<int>> connfds;//用于传递连接给子线程的共享变量
    std::mutex newConnectionMutex;//用于分配连接给工作线程时使用的锁
    static const int MAX_CONNS=10000;//单个线程管理的最大连接数
};


#endif //HTTP_SERVER_THREADPOOL_H
