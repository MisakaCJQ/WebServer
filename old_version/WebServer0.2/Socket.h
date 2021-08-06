//
// Created by cjq on 7/25/21.
//
#include <iostream>
#include <string>
#ifndef HTTP_SERVER_SOCKET_H
#define HTTP_SERVER_SOCKET_H


class Socket{
public:
    Socket();
    explicit Socket(int _fd):fd(_fd){};
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
private:
    int fd;
    static const unsigned int MAX_BUF_SIZE=512;
};


#endif //HTTP_SERVER_SOCKET_H
