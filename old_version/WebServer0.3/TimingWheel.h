//
// Created by cjq on 8/2/21.
//
#include <unordered_set>
#include <vector>
#include <memory>
#include "Socket.h"
#ifndef HTTP_SERVER_TIMINGWHEEL_H
#define HTTP_SERVER_TIMINGWHEEL_H


class TimingWheel {
public:
    explicit TimingWheel(int _bucketNum)
            :bucketNum(_bucketNum),tail(0),aliveSocketNum(0),
            whell(std::vector<std::unordered_set<std::shared_ptr<Socket>>>(_bucketNum)){};
    void addSocket(const std::shared_ptr<Socket>& socketPtr);
    void rotate();
    unsigned int getaliveSocketNum() const;
private:
    int bucketNum;
    int tail;
    unsigned int aliveSocketNum;
    std::vector<std::unordered_set<std::shared_ptr<Socket> > > whell;
};


#endif //HTTP_SERVER_TIMINGWHEEL_H
