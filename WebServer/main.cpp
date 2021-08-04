
#include <getopt.h>
#include <iostream>
#include "MyHttpServer.h"
#include "Socket.h"
using namespace std;
const option long_options[]={
        {"ip",required_argument,nullptr,'i'},
        {"port",required_argument, nullptr,'p'},
        {"thread-number",required_argument, nullptr,'t'},
        {nullptr,0,nullptr,0}
};

int main(int argc, char* argv[]) {

    string ip="0.0.0.0";
    int port=9833;
    int threadNum=1;

    int opt;
    while((opt= getopt_long(argc,argv,"i:p:t:",long_options, nullptr))!=-1)
    {
        if(opt=='i')
            ip=optarg;
        else if(opt=='p')
            port= stoi(optarg);
        else if (opt=='t')
            threadNum= stoi(optarg);
        else
            exit(EXIT_FAILURE);
    }
    cout<<"Server Information:"<<endl;
    cout<<"ip:"<<ip<<' '<<"port:"<<port<<" thread num:"<<threadNum<<endl<<endl;

    MyHttpServer aServ(ip,port,threadNum);
    aServ.startup();
    return 0;
}

