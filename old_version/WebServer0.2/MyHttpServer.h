//
// Created by cjq on 5/2/21.
//
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "HttpProcessor.h"
using namespace std;
#ifndef HTTP_SERVER_MYHTTPSERVER_H
#define HTTP_SERVER_MYHTTPSERVER_H

class MyHttpServer {
public:
    MyHttpServer():ip("0.0.0.0"),port(9877),threadNum(1){}
    MyHttpServer(std::string  _ip,int _port,int _threadNum)
        :ip(std::move(_ip)),port(_port),threadNum(_threadNum){}
    [[noreturn]] void startup();
private:
    string ip;
    int port;
    const int threadNum;
};


#endif //HTTP_SERVER_MYHTTPSERVER_H
