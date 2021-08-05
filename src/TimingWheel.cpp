//
// Created by cjq on 8/2/21.
//
#include "TimingWheel.h"

/*
 * 向时间轮中加入新的需要管理的Socket
 * 可以重复
 */
void TimingWheel::addSocket(const std::shared_ptr<Socket>& socketPtr)
{
    if(whell[tail].count(socketPtr)==0)
    {//当前bucket没有这一socket就加入
        whell[tail].insert(socketPtr);
        aliveSocketNum++;
    }
}

/*
 * 旋转时间轮
 * tail指针移动
 * 然后清空指向的的set
 */
void TimingWheel::rotate()
{
    tail++;
    if(tail==bucketNum)
        tail=0;

    aliveSocketNum-=whell[tail].size();
    whell[tail].clear();
}

/*
 * 返回当前时间轮中的Socket数量
 */
unsigned int TimingWheel::getaliveSocketNum() const
{
    return aliveSocketNum;
}
