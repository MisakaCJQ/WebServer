#include <getopt.h>
#include <iostream>
#include <thread>
#include "MyHttpServer.h"
using namespace std;
const option long_options[]={
        {"ip",required_argument,nullptr,'i'},
        {"port",required_argument, nullptr,'p'},
        {nullptr,0,nullptr,0}
};

int main(int argc, char* argv[]) {

    string ip="127.0.0.1";
    int port=9833;

    int opt;
    while((opt= getopt_long(argc,argv,"i:p:",long_options, nullptr))!=-1)
    {
        if(opt=='i')
            ip=optarg;
        else if(opt=='p')
            port= stoi(optarg);
        else
            exit(EXIT_FAILURE);
    }
    cout<<"Server Information:"<<endl;
    cout<<"ip:"<<ip<<' '<<"port:"<<port<<endl<<endl;

    MyHttpServer aServ(ip,port);
    aServ.startup();
    return 0;
}
