#ifndef __HRRPSERVER_HPP__
#define __HTTPSERVER_HPP__
#include<pthread.h>
#include"protocolutil.hpp"
#include"threadpool.hpp"
#include"Log.hpp"
class httpserver
{
    private:
    int  listen_sock;
    int port;
    ThreadPool*tp;

    public:
    httpserver(int _port):port(_port),listen_sock(-1),tp(NULL)
    {}
    void InitServer()
    {
        listen_sock=socket(AF_INET,SOCK_STREAM,0);
        if(listen_sock<0)
        {
        LOG(ERROR,"create socket error!");
        exit(2);
        }

        int opt=1;
        setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));//若服务器挂掉可立即重启并不会进入timewait状态

        struct sockaddr_in local_;
        local_.sin_family=AF_INET;
        local_.sin_port=htons(port);
        local_.sin_addr.s_addr=INADDR_ANY;//
        if(bind(listen_sock,(struct sockaddr*)&local_,sizeof(local_))<0)
        {
            LOG(ERROR,"bind socket error!");
            exit(3);
        }
        if(listen(listen_sock,5)<0)
        {
            LOG(ERROR,"listen socket error!");
            exit(4);
        }
        LOG(INFO,"initserver success!");
            tp=new ThreadPool();
            tp->initThreadPool();
        }
            
        void Start()
        {
            LOG(INFO,"start server begin");
            while(1)
            {
            struct sockaddr_in peer_;
            socklen_t len=sizeof(peer_);
            int sock=accept(listen_sock,(sockaddr*)&peer_,&len);
            if(sock<0)
            {
                LOG(WARNING,"accept error!");
                continue;
            }
            Task t;
            t.SetTask(sock,Entry::HanderRequest);
            tp->PushTask(t);
            }

        }
        ~httpserver()
        {
            if(listen_sock>0)
            {
                close(listen_sock);
            }
            port=-1;
        }
};
#endif
