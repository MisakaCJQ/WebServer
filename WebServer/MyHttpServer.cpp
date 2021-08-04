//
// Created by cjq on 5/2/21.
//

#include "MyHttpServer.h"
#include "Socket.h"
#include "ThreadPool.h"

[[noreturn]] void MyHttpServer::startup()
{
    //创建监听socket
    Socket listenSocket;
    listenSocket.setReuseAddr();
    listenSocket.Bind(ip, port);
    listenSocket.Listen(10000);

    //创建线程池
    ThreadPool threadPool(threadNum);

    //循环监听并将连接fd加入线程池
    while(true)
    {
        int connfd=listenSocket.Accept();
        threadPool.addNewConnectionEvent(connfd);
    }
}
