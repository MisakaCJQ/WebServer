//
// Created by cjq on 7/20/21.
//

#include "HttpProcessor.h"
using namespace std;

/*
 * 主要的处理函数
 * 选择不同的方法处理函数
 * 会通过报文判断是否为持久连接，为持久连接则返回1,否则返回0
 */
int HttpProcessor::handleMethod(const std::string &message,std::string &response) {
    vector<vector<string>> split_message= parseMessage(message);//对报文进行分割
    bool isAlive=isAliveConnection(split_message);

    if(split_message[0][0] == "GET")
        response=handleGet(split_message,isAlive);
    else if(split_message[0][0] == "POST")
        response=handlePost(split_message,isAlive);
    else
        response=handleOther(split_message[0][0],isAlive);

    if(isAlive)
        return 1;
    else
        return 0;
}

/*----------------------------------------------------------------------------------------*/

/*
 * 处理GET方法
 * 读取文件并返回HTTP响应报文
 */
std::string HttpProcessor::handleGet(const vector<std::vector<std::string>> &split_message, bool isAlive) {
    string filedata;
    if(getFileData(split_message[0][1], filedata))
        return ok200header(filedata.size(),isAlive)+filedata;
    else//url不存在，404
        return notFound404header(filedata.size(),isAlive)+filedata;
}

/*
 * 处理POST方法
 * 主要是实现一个回显功能
 */
std::string HttpProcessor::handlePost(const vector<std::vector<std::string>> &split_message, bool isAlive) {
    string message,name,id,filedata;
    if(split_message[0][0]!="POST" ||
       split_message[0][1]!="/Post_show" ||
       !getNameAndID(split_message.back()[1],name,id))
    {
        getFileData("/404_not_found.html",filedata);
        message=notFound404header(filedata.size(),isAlive)+filedata;
        return message;
    }

    getFileData("/post_show.html",filedata);
    filedata+="<body>\n<h1>Your Name:"+
              name+"</h1>\n<h1>Your ID:"+
              id+"</h1></body>\n</html>\n";
    message= ok200header(filedata.size(),isAlive)+filedata;
    return message;
}

/*
 * 处理其他方法
 * 直接返回501错误
 */
std::string HttpProcessor::handleOther(const std::string &method, bool isAlive) {
    string filedata;
    getFileData("/501_not_implemented.html",filedata);
    filedata+=method+"</p>\n</body>\n</html>\n";
    return notImplemented501header(filedata.size(),isAlive)+filedata;
}

/*
 * 200响应报文头部
 */
std::string HttpProcessor::ok200header(unsigned int dataLength, bool isAlive)
{
    string header;
    header+="HTTP/1.1 200 OK\r\n";
    header+="Server: MyHttpServer\r\n";
    header+="Connection: ";
    if(isAlive)
        header+="Keep-Alive\r\n";
    else
        header+="Close\r\n";
    header+= "Content-length: " + to_string(dataLength) + "\r\n";
    header+="\r\n";
    return header;
}

/*
 * 404响应报文头部
 */
std::string HttpProcessor::notFound404header(unsigned int dataLength, bool isAlive) {
    string header;
    header+="HTTP/1.1 404 Not Found\r\n";
    header+="Server: MyHttpServer\r\n";
    header+="Connection: ";
    if(isAlive)
        header+="Keep-Alive\r\n";
    else
        header+="Close\r\n";
    header+= "Content-length: " + to_string(dataLength) + "\r\n";
    header+="\r\n";
    return header;
}

/*
 * 501响应报文头部
 */
std::string HttpProcessor::notImplemented501header(unsigned int dataLength, bool isAlive) {
    string header;
    header+="HTTP/1.1 501 Not Implemented\r\n";
    header+="Server: MyHttpServer\r\n";
    header+="Connection: ";
    if(isAlive)
        header+="Keep-Alive\r\n";
    else
        header+="Close\r\n";
    header+= "Content-length: " + to_string(dataLength) + "\r\n";
    header+="\r\n";
    return header;
}

/*---------------------------------------------------------------------------------*/

/*
 * 用于POST方法的主要函数。
 * 正确从data字段中读出格式正确的name和id则返回true,否则false
 * 通过name和id两个参数传结果
 */
bool HttpProcessor::getNameAndID(const string &data, string &name, string &id) {
    auto andPos=data.find('&');
    if(andPos==string::npos)
        return false;

    //提取name部分字段
    string namePart=data.substr(0,andPos);
    string idPart=data.substr(andPos+1);
    if(namePart.find('&')!=string::npos || idPart.find('&')!=string::npos)
        return false;

    //提取name和id字段的key和value
    auto nameEqualPos=namePart.find('=');
    auto idEqualPos=idPart.find('=');
    if(nameEqualPos==string::npos || idEqualPos==string::npos)
        return false;
    string nameKey=namePart.substr(0,nameEqualPos);
    string nameValue=namePart.substr(nameEqualPos+1);
    string idKey=idPart.substr(0,idEqualPos);
    string idValue=idPart.substr(idEqualPos+1);
    //检查key是否是name和ID
    if(nameKey!="Name" || idKey!="ID" ||nameValue.empty()||idValue.empty())
        return false;

    name=nameValue;
    id=idValue;
    return true;
}

/*
 * 用于读取文件
 * 路径错误或打开文件失败都会返回false，否则返回true
 * 通过参数data传文件读取的结果
 */
bool HttpProcessor::getFileData(const string &filepath, string &data) {
    //路径文件不存在会返回false，否则返回true
    ifstream in;
    if(!isFilePath("./server"+filepath))//如果给的路径是目录
    {
        if(filepath.back()=='/')
            in.open("./server"+filepath+"index.html");
        else
            in.open("./server"+filepath+"/index.html");
    }
    else
        in.open("./server"+filepath);

    if(!in)//文件打开失败，返回false
    {
        in.open("./server/404_not_found.html");
        stringstream ss;
        ss << in.rdbuf();
        data=ss.str();
        return false;
    }
    else
    {
        stringstream ss;
        ss << in.rdbuf();
        data=ss.str();
        return true;
    }
}

/*
 * 用于判断一个路径是目录还是文件
 * 文件路径返回true，目录则返回false
 */
bool HttpProcessor::isFilePath(const string &path) {
    struct stat path_stat{};
    stat(path.c_str(),&path_stat);
    return S_ISREG(path_stat.st_mode);
}

/*
 * 将HTTP报文分解为二维vector
 * 主要通过空格进行分割
 */
std::vector<std::vector<std::string>> HttpProcessor::parseMessage(const string &data) {
    stringstream messageSS(data);
    vector<vector<string>> datas;

    //首先对第一行首部进行处理
    string temp;
    getline(messageSS, temp);
    temp.pop_back();//将尾部的\r去掉
    //寻找前后两个空格的位置
    auto sp1=temp.find_first_of(' ');
    auto sp2=temp.find_last_of(' ');
    //分割并插入
    datas.push_back({temp.substr(0, sp1),
                     temp.substr(sp1+1,sp2-sp1-1),
                     temp.substr(sp2+1)});

    //循环处理剩余的报文头部
    while(getline(messageSS, temp))
    {
        temp.pop_back();//将尾部的\r去掉
        auto sp=temp.find(' ');//寻找中间的空格
        if(sp!=string::npos)
            datas.push_back({temp.substr(0, sp),
                             temp.substr(sp+1)});
        else//找不到空格，说明是\r空行。
            break;
    }
    //读取报文中最后的data部分
    while(getline(messageSS,temp,'\0'));
    datas.push_back({"msdata:",temp});//在解析结果尾部加上一行数据

    return datas;
}

/*
 * 通过HTTP报文内容判断是否为持久连接
 */
bool HttpProcessor::isAliveConnection(const vector<std::vector<std::string>> &split_message) {
    for(const vector<string> &temp:split_message)
    {
        if(temp[0]=="Connection:")
        {
            if(temp[1]=="Keep-Alive")
                return true;
            else
                return false;
        }
    }
    if(split_message[0][2]=="HTTP/1.0")
        return false;
    else
        return true;
}
