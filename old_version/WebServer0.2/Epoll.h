//
// Created by cjq on 7/29/21.
//
#include <unordered_map>
#include <vector>
#include <memory>
#include "Socket.h"
#ifndef HTTP_SERVER_EPOLL_H
#define HTTP_SERVER_EPOLL_H

enum EVENT_TYPE{RECVABLE,SENDABLE};
struct myEvent
{
    std::shared_ptr<Socket> socketPtr= nullptr;
    EVENT_TYPE type;
};

class Epoll {
public:
    explicit Epoll(int maxConns);
    void addRecvEvent(const std::shared_ptr<Socket>& socketPtr);
    void addSendEvent(const std::shared_ptr<Socket>& socketPtr);
    void resetRecvOneshot(const std::shared_ptr<Socket>& socketPtr);
    void resetSendOneshot(const std::shared_ptr<Socket>& socketPtr);
    void deleteEvent(const std::shared_ptr<Socket>& socketPtr);
    int EpollWait(std::vector<myEvent> &myEvents);
private:
    Socket epfdSocket;
    std::unordered_map<int,std::shared_ptr<Socket>> Sockets;
    static const int MAX_EVENT_NUM=10000;
};


#endif //HTTP_SERVER_EPOLL_H
