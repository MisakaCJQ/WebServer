//
// Created by cjq on 5/2/21.
//
#include <string>
#include "HttpProcessor.h"
using namespace std;
#ifndef HTTP_SERVER_MYHTTPSERVER_H
#define HTTP_SERVER_MYHTTPSERVER_H

class MyHttpServer {
public:
    MyHttpServer():ip("0.0.0.0"),port(9877),threadNum(1){}//默认为单线程
    MyHttpServer(std::string  _ip,int _port,int _threadNum)
        :ip(std::move(_ip)),port(_port),threadNum(_threadNum){}
    [[noreturn]] void startup();
private:
    std::string ip;
    int port;
    const int threadNum;
};


#endif //HTTP_SERVER_MYHTTPSERVER_H
