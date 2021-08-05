//
// Created by cjq on 7/25/21.
//
#include <string>
#include <unordered_map>
#include <memory>
#ifndef HTTP_SERVER_SOCKET_H
#define HTTP_SERVER_SOCKET_H


class Socket{
public:
    Socket();
    explicit Socket(int _fd):fd(_fd),isAlive(true),recvingBufferPtr(nullptr){};
    ~Socket();
    void setReuseAddr() const;
    void setNonBlocking() const;
    void Bind(const std::string& ip,int port) const;
    void Listen(int maxConnNum) const;
    int Accept() const;
    int getMessage(std::string& data);
    ssize_t sendMessage(const std::string& data);
    void readUint64();
    int getFD() const;
    void shutdown();
private:
    int fd;
    bool isAlive;
    static const unsigned int MAX_BUF_SIZE=512;
    /*用于在析构时清除的指针，不是动态内存所以没用智能指针*/
    std::unordered_map<int,std::string>* recvingBufferPtr;
};


#endif //HTTP_SERVER_SOCKET_H
