#include<unistd.h>
#include"httpserver.hpp"
void Usage(std::string proc)
{
    std::cout<<"Usage "<<proc<<"port"<<std::endl;
}
int main(int argc,char*argv[])
{
    if(argc!=2)
    {
        Usage(argv[0]);//argv[0]传的是文件名
        exit(1);
    }
    httpserver *serp=new httpserver(atoi(argv[1]));
    serp->InitServer();
    serp->Start();
    delete  serp;
    return 0;
}
