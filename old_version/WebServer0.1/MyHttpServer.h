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
using namespace std;
#ifndef HTTP_SERVER_MYHTTPSERVER_H
#define HTTP_SERVER_MYHTTPSERVER_H

class MyHttpServer {
public:
    void startup();
    MyHttpServer():ip("0.0.0.0"),port(9877){}
    explicit MyHttpServer(std::string  _ip,int _port):ip(std::move(_ip)),port(_port){}
private:
    void handleConnection(int connfd,sockaddr_in cliaddr,int count);
    int handleHttpRequest(int connfd,sockaddr_in cliaddr);
    int getMessage(int connfd,string &data) const;
    vector<vector<string>> parseMessage(const string &data);
    string handleGet(const vector<vector<string>> &split_message,bool isAlive);
    string handlePost(const vector<vector<string>> &split_message,bool isAlive);
    string handleOther(const string &method,bool isAlive);
    bool getFileData(const string &filepath,string &data);
    string notFound404header(unsigned int dataLength,bool isAlive);
    string ok200header(unsigned int dataLength,bool isAlive);
    string notImplemented501header(unsigned dataLength,bool isAlive);
    bool getNameAndID(const string &data,string &name,string &id);
    bool isAliveConnection(const vector<vector<string>> &split_message);
private:
    string ip;
    int port;
    const unsigned int MAX_BUF_SIZE=512;
};


#endif //HTTP_SERVER_MYHTTPSERVER_H
