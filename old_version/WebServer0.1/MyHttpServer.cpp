#include "MyHttpServer.h"
using namespace std;
bool isFilePath(const string& path)
{
    struct stat path_stat{};
    stat(path.c_str(),&path_stat);
    return S_ISREG(path_stat.st_mode);
}

void MyHttpServer::startup()
{
    //设置服务器socket地址
    sockaddr_in servaddr{};
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;//设定ipv4协议
    inet_pton(AF_INET,ip.c_str(),&servaddr.sin_addr);//设定地址
    servaddr.sin_port= htons(port);//设定端口

    //获取监听socket
    int listenfd= socket(AF_INET,SOCK_STREAM,0);
    if(bind(listenfd,(sockaddr*)&servaddr,sizeof(servaddr))<0)
    {//将socket绑定到地址上
        cout<<"wrong bind!"<<endl;
        cout<<"Port "<<port<<" might be occupied"<<endl;
        return ;
    }

    listen(listenfd,10);//开始监听
    cout<<"start listening...\n";
    for(int i=0;;i++)
    {
        sockaddr_in cliaddr{};
        socklen_t clilen=sizeof(cliaddr);
        //开始等待连接
        int connfd= accept(listenfd,(sockaddr*)&cliaddr,&clilen);
        thread t(&MyHttpServer::handleConnection,this,connfd,cliaddr,i);
        t.detach();
        //handleConnection(connfd,cliaddr);
        //close(connfd);
    }

}

void MyHttpServer::handleConnection(int connfd,sockaddr_in cliaddr,int count)
{
    std::ios::sync_with_stdio(false);
    string cliIp= inet_ntoa(cliaddr.sin_addr);
    int cliPort= ntohs(cliaddr.sin_port);
    //cout<<count<<"--connection from:"<<cliIp<<' '<<cliPort<<endl;
    while(true)
    {
        int flag= handleHttpRequest(connfd,cliaddr);
        if(flag==0||flag==-1)
        {
            //cout<<count<<"--connection close"<<endl;
            close(connfd);
            return;
        }
    }
}

int MyHttpServer::handleHttpRequest(int connfd,sockaddr_in cliaddr)
{
    string data;
    int flag=getMessage(connfd,data);//获取报文信息
    if(flag==0||flag==-1)//如果客户端已经断开连接或是发生错误，则直接返回
        return flag;

    //cout<<data<<endl;
    vector<vector<string>> split_message= parseMessage(data);//对报文进行分割

    string message;
    bool isAliveConn=isAliveConnection(split_message);
    if(split_message[0][0] == "GET")
        message = handleGet(split_message,isAliveConn);
    else if(split_message[0][0] == "POST")
        message= handlePost(split_message,isAliveConn);
    else
        message= handleOther(split_message[0][0],isAliveConn);

    send(connfd,message.c_str(),message.size(),0);

    /*判断是否为长连接*/
    if(isAliveConn)
        return flag;
    else
        return 0;
}

int MyHttpServer::getMessage(int connfd,string &data) const
{
    vector<char> buf(MAX_BUF_SIZE);//定义缓冲区
    ssize_t byterecv=0;
    //循环读取数据
    do
    {
        byterecv= recv(connfd,&buf[0],MAX_BUF_SIZE,0);
        if(byterecv<0)
        {
            cout<<"wrong recv!\n";
            return -1;
        }
        else if (byterecv==0)//客户端关闭连接
            return 0;
        else
            data.append(buf.cbegin(),buf.cbegin()+byterecv);
    }
    while(byterecv==MAX_BUF_SIZE);
    return 1;//正常收到了一份HTTP报文
}

vector<vector<string>> MyHttpServer::parseMessage(const string &data) {
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
    while(getline(messageSS,temp,'\0'));
    datas.push_back({"msdata:",temp});//在解析结果尾部加上一行数据

    return datas;
}

string MyHttpServer::handleGet(const vector<vector<string>> &split_message,bool isAlive) {
    string filedata;
    if(getFileData(split_message[0][1], filedata))
        return ok200header(filedata.size(),isAlive)+filedata;
    else
        return notFound404header(filedata.size(),isAlive)+filedata;
}

string MyHttpServer::handlePost(const vector<vector<string>> &split_message,bool isAlive) {
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

string MyHttpServer::handleOther(const string &method,bool isAlive) {
    string filedata;
    getFileData("/501_not_implemented.html",filedata);
    filedata+=method+"</p>\n</body>\n</html>\n";
    return notImplemented501header(filedata.size(),isAlive)+filedata;
}

bool MyHttpServer::getFileData(const string &filepath, string &data) {
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

string MyHttpServer::notFound404header(unsigned int dataLength,bool isAlive) {
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

string MyHttpServer::ok200header(unsigned int dataLength,bool isAlive) {
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

string MyHttpServer::notImplemented501header(unsigned int dataLength,bool isAlive) {
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

bool MyHttpServer::getNameAndID(const string &data, string &name, string &id) {
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

bool MyHttpServer::isAliveConnection(const vector<vector<string>> &split_message) {
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




