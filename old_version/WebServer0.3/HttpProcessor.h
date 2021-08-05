//
// Created by cjq on 7/20/21.
//
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#ifndef HTTP_SERVER_HTTPPROCESSOR_H
#define HTTP_SERVER_HTTPPROCESSOR_H


class HttpProcessor {
public:
    static int handleMethod(const std::string &message,std::string &response);
private:
    static std::string handleGet(const std::vector<std::vector<std::string>> &split_message,bool isAlive);
    static std::string handlePost(const std::vector<std::vector<std::string>> &split_message,bool isAlive);
    static std::string handleOther(const std::string &method,bool isAlive);
    static std::string ok200header(unsigned int dataLength,bool isAlive);
    static std::string notFound404header(unsigned int dataLength,bool isAlive);
    static std::string notImplemented501header(unsigned dataLength,bool isAlive);
private:
    static std::vector<std::vector<std::string>> parseMessage(const std::string &data);
    static bool getNameAndID(const std::string &data,std::string &name,std::string &id);
    static bool getFileData(const std::string &filepath,std::string &data);
    static bool isFilePath(const std::string& path);
    static bool isAliveConnection(const std::vector<std::vector<std::string>> &split_message);
};


#endif //HTTP_SERVER_HTTPPROCESSOR_H
