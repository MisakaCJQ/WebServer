//
// Created by cjq on 7/29/21.
//
#include <unordered_map>
#include <vector>
#include <memory>
#include "Socket.h"
#include "TimingWheel.h"
#ifndef HTTP_SERVER_EPOLL_H
#define HTTP_SERVER_EPOLL_H

enum EVENT_TYPE{RECVABLE,SENDABLE,NEW_CONN,TIMER};
struct myEvent
{
    std::shared_ptr<Socket> socketPtr= nullptr;
    EVENT_TYPE type;
};

class Epoll {
public:
    explicit Epoll(int maxConns,int expiredTime);
    void setEventfd(const std::shared_ptr<Socket>& socketPtr);
    void addRecvEvent(const std::shared_ptr<Socket>& socketPtr);
    void addSendEvent(const std::shared_ptr<Socket>& socketPtr);
    void resetEventfdOneshot();
    void resetRecvOneshot(const std::shared_ptr<Socket>& socketPtr);
    void resetSendOneshot(const std::shared_ptr<Socket>& socketPtr);
    void deleteEvent(const std::shared_ptr<Socket>& socketPtr);
    int EpollWait(std::vector<myEvent> &myEvents);
    void rotateTimingWhell();
    void tryStopTimer();
private:
    void tryStartTimer();
    void resetTimerfdOneshot();
private:
    Socket epfdSocket;
    std::shared_ptr<Socket> eventfdSocketPtr;
    std::shared_ptr<Socket> timerfdSocketPtr;
    std::unordered_map<int,std::weak_ptr<Socket>> Sockets;
    TimingWheel timingWheel;
    static const int MAX_EVENT_NUM=10000;
    bool isTimerStart;
};


#endif //HTTP_SERVER_EPOLL_H
