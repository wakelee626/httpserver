//GET方法和POST方法
//GET方法可以传参可以不传参，POST方法都要传参
//GET方法通过url传参POST方法通过正文传参


#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__
#include<string>
#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sstream>
#include<strings.h>
#include<string.h>
#include<sys/stat.h>
#include<unordered_map>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<sys/wait.h>
#include"Log.hpp"

#define NOT_FOUND 404
#define BAD_REQUEST 400
#define SERVER_ERROR 500
#define OK 200

#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define PAGE_404 "404.html"
#define HTTP_VERSION "http/1.0"

std::unordered_map<std::string,std::string> stuffix_map
{
    {".html","text/html"},
        {".htm","text/html"},
        {".css","text/css"},
        {"js","application/x-javascript"}
};

class ProtocolUtil
{
    public:
        static void MakeKV(std::unordered_map<std::string,std::string>&kv,std::string &str)
        {
            std::size_t pos=str.find(": ");
            if(std::string::npos==pos)
            {
                return;
            }
            std::string k=str.substr(0,pos);
            std::string v=str.substr(pos+2);
            kv.insert(make_pair(k,v));
        }
        static std::string IntToString(int code)
        {
            std::stringstream ss;
            ss<<code;
            return ss.str();
        }
        static std::string  CodeToDesc(int code)
        {
            switch(code)
            {
                case 200:
                    return "Ok";
                case 404:
                    return "Not Found";
                case 400:
                    return "Bad Request";
                case 500:
                    return "Internal Server Error";
                default:
                    return "Unknow";
            }
        }
        static std::string SuffixToType(const std::string &suffix)
        {
            return stuffix_map[suffix];
        }
};

class Request
{
    public:
        std::string rq_line;
        std::string rq_head;
        std::string blank;
        std::string rq_text;
    public:
        std::string method;
        std::string uri;
        std::string version;
        bool cgi;//method=POST,GET->uri(?)
        std::string path;
        std::string param;
        int resource_size;
        std::string resource_suffix;
        int content_length;
        std::unordered_map<std::string,std::string> head_kv;
    public:
        Request():blank("\n"),cgi(false),path(WEB_ROOT),resource_size(0),content_length(-1),resource_suffix(".html")
    {}
        int GetResourceSize()
        {
            return resource_size;
        }
        void SetResourceSize(int rs)
        {
            resource_size=rs;
        }
        std::string &GetSuffix()
        {
            return resource_suffix;
        }
        void SetSuffix(std::string suffix)
        {
            resource_suffix=suffix;
        }
        std::string &GetParam()
        {
            return param;
        }
        std::string &GetPath()
        {
            return path;
        }
        void SetPath(std::string &path_)
        {
            path=path_;
        }
        void RequestLineParse()//分析一行
        {
            std::stringstream ss(rq_line);
            ss>>method>>uri>>version;
            std::cout<<method<<std::endl;
            std::cout<<uri<<std::endl;
            std::cout<<version<<std::endl;
        }
        void UriParse()
        {
            if(strcasecmp(method.c_str(),"GET")==0)
            {
                std::size_t pos=uri.find('?');
                if(std::string::npos!=pos)
                {
                    cgi=true;
                    path += uri.substr(0,pos);
                    param = uri.substr(pos+1);
                }
                else
                {
                    path+=uri;
                }
            }
            else{
                path+=uri;
            }

            if(path[path.size()-1]=='/')
            {
                path+= HOME_PAGE;
            }
        }
        bool RequestHeadParse()
        {
            int start=0;
            while(start<rq_head.size())
            {
                std::size_t pos=rq_head.find('\n',start);
                if(std::string::npos==pos)
                {
                    break;
                }
                std::string sub_string=rq_head.substr(start,pos-start);
                if(!sub_string.empty())
                {
                    ProtocolUtil::MakeKV(head_kv,sub_string);
                }
                else
                {
                    break;
                }
                start=pos+1;
            }
            return true;
        }
        int GetContentLength()
        {
            std::string cl=head_kv["Content-Length"];
            if(!cl.empty())
            {
                std::stringstream ss(cl);
                ss>>content_length;
            }
            return content_length;
        }
        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(),"GET")==0||(cgi=(strcasecmp(method.c_str(),"POST")==0)))//strcasecmp忽略大小写的字符串比较
            {
                return true;
            }
            return false;
        }
        bool IsPathLegal()
        {
            struct stat st;
            if(stat(path.c_str(),&st)<0)
            {
                LOG(WARNING,"path not found!");
                return false;
            }
            if(S_ISDIR(st.st_mode))
            {
                path+='/';
                path+=HOME_PAGE;
            }
            else
            {
                if((st.st_mode&S_IXUSR)||(st.st_mode&S_IXGRP)||(st.st_mode&S_IXOTH))
                {
                    cgi=true;
                }
            }
            resource_size=st.st_size;
            std::size_t pos=path.rfind('.');
            if(std::string::npos!=pos)
            {
                resource_suffix=path.substr(pos);
            }
            return true;
        }

        bool IsNeedRecvText()
        {
            if(strcasecmp(method.c_str(),"POST")==0)
            {
                return true;
            }
            return false;
        }
        bool IsCgi()
        {
            return cgi;
        }
        ~Request()
        {}
};
class Response
{
    public:
        std::string rsp_line;
        std::string rsp_head;
        std::string blank;
        std::string rsp_text;
        int fd;
    public:
        int code;//状态码
    public:
        Response():blank("\n"),code(OK)
    {}
        void MakeStatusLine()
        {
            rsp_line=HTTP_VERSION;
            rsp_line+=" ";
            rsp_line+=ProtocolUtil::IntToString(code);
            rsp_line+=" ";
            rsp_line+=ProtocolUtil::CodeToDesc(code);
            rsp_line+="\n";
        }
        void  MakeResponseHead(Request *&rq)
        {
            rsp_head="Content-Length: ";
            rsp_head+=ProtocolUtil::IntToString(rq->GetResourceSize());
            rsp_head+="\n";
            rsp_head+="Content-Type: ";
            std::string suffix_=rq->GetSuffix();
            rsp_head+=ProtocolUtil::SuffixToType(suffix_);
            rsp_head+="\n";
        }
        void OpenResource(Request *&rq)
        {
            std::string path_=rq->GetPath();
            std::cout<<path_<<std::endl;
            fd=open(path_.c_str(),O_RDONLY);
        }

        ~Response()
        {
            if(fd>=0)
            {
                close(fd);
            }
        }
}; 

class Connect
{
    private:
        int sock;
    public:
        Connect(int sock_):sock(sock_)
    {}
        int RecvOneLine(std::string &line)
        {
            //换行符有三种： \n，\r，\r\n
            //要把三种情况转成一种，要将\r转为\n
            char c='X';
            while(c!='\n')
            {
                ssize_t s=recv(sock,&c,1,0);
                if(s>0)
                {
                    if(c=='\r')
                    {
                        //recv窥探功能MSG_PEEK来窥探/r后的字符是不是/n
                        recv(sock,&c,1,MSG_PEEK);
                        if(c=='\n')
                        {
                            recv(sock,&c,1,0);
                        }
                        else
                        {
                            c='\n';
                        }
                    }
                    line.push_back(c);
                }
                else
                {
                    break;
                }
            }
            return line.size();
        }
        void RecvRequestHead(std::string &head)
        {
            head="";
            std::string  line;
            while(line!="\n")
            {
                line="";
                RecvOneLine(line);
                head+=line;
            }
        }
        void RecvRequestText(std::string &text,int len,std::string &param)
        {
        std::cout<<"maketext"<<std::endl;
            char c;
            int i=0;
            while(i<len)
            {
                recv(sock,&c,1,0);
                text.push_back(c);
                i++;
            }
            std::cout<<text<<std::endl;
            std::cout<<param<<std::endl;
            param=text;
        }
        void SendResponse(Response *&rsp,Request *&rq,bool cgi)
        {
            std::string &rsp_line_=rsp->rsp_line;
            std::cout<<rsp_line_<<" ";
            std::string &rsp_head_=rsp->rsp_head;
            std::cout<<rsp_head_<<" ";
            std::string &blank_=rsp->blank;

            send(sock,rsp_line_.c_str(),rsp_line_.size(),0);
            send(sock,rsp_head_.c_str(),rsp_head_.size(),0);
            send(sock,blank_.c_str(),blank_.size(),0);
            if(cgi) 
            { 
                std::string &rsp_text=rsp->rsp_text;
                send(sock,rsp_text.c_str(),rsp_text.size(),0);
            }
            else{
                int &fd=rsp->fd;
                sendfile(sock,fd,NULL,rq->GetResourceSize());
            }
        }

        ~Connect()
        {
            if(sock>=0)
            {
                close(sock);
            }
        }
};

class Entry
{
    public:
        static void ProcessNonCgi(Connect *&conn,Request *&rq,Response*&rsp)
        {
            int &code_=rsp->code; 
            rsp->MakeStatusLine();
            rsp->MakeResponseHead(rq);
            rsp->OpenResource(rq);
            conn->SendResponse(rsp,rq,false);
        }
        static void ProcessCgi(Connect *&conn,Request *&rq,Response *&rsp)
        {
            int &code=rsp->code;
            int input[2];
            int output[2];
            std::string &param=rq->GetParam();
            std::cout<<param<<std::endl;
            std::string &rsp_text=rsp->rsp_text;
            pipe(input);
            pipe(output);
            pid_t id=fork();
            if(id<0)
            {
                //  LOG("ERROR",);
                code=SERVER_ERROR;
                return;
            }
            else if(id==0)
            {//child
                close(input[1]);
                close(output[0]);
                std::string &path=rq->GetPath();
                std::string cl_env="Content-Length=" ;
                cl_env+=ProtocolUtil::IntToString(param.size());
                putenv((char*)cl_env.c_str());

                dup2(input[0],0);
                dup2(output[1],1);

                execl(path.c_str(),path.c_str(),NULL);
                exit(1);
            }
            else
            {//parent
                close(input[0]);
                close(output[1]);
                size_t total=0;//已经累计写多少
                size_t size=param.size();//期望写多少
                size_t curr=0;//实际写多少
                const char *p=param.c_str();

                while(total<size&&(curr=write(input[1],p+total,size-total))>0)
                {
                    total+=curr;
                }

                char c;
                while(read(output[0],&c,1)>0)
                {
                    rsp_text.push_back(c);
                }
                waitpid(id,NULL,0);
                close(input[1]);
                close(output[0]);

                rsp->MakeStatusLine();
                rq->SetResourceSize(rsp_text.size());
                rsp->MakeResponseHead(rq);
                conn->SendResponse(rsp,rq,true);
            }

        }
        static int  ProcessResponse(Connect *&conn,Request *&rq,Response *&rsp)
        {
            if(rq->IsCgi())
            {
                ProcessCgi(conn,rq,rsp);
            }
            else
            {
                ProcessNonCgi(conn,rq,rsp);
            }
        }
        static void Process404(Connect *&conn,Request *&rq,Response *&rsp)
        {
            std::cout<<"int to p404"<<std::endl;
            std::string path=WEB_ROOT;
            path+="/";
            path+=PAGE_404;
            struct stat st;
            stat(path.c_str(),&st);
            rq->SetResourceSize(st.st_size);
            rq->SetSuffix(".html"); 
            rq->SetPath(path);
            ProcessNonCgi(conn,rq,rsp);    
        }
        static void HanderError(Request *&rq,Response *&rsp,Connect *&conn)
        {
        std::cout<<"into hander"<<std::endl;
            int &code=rsp->code;
            switch(code)
            {
                case 400:
                    break;
                case 404:
                    Process404(conn,rq,rsp);
                    break;
                case 500:
                    break;
                case 503:
                    break;
            }
        }
        //处理请求
        static int HanderRequest(int sock_)
        {
            Connect *conn=new Connect(sock_);
            Request *rq=new Request();
            Response *rsp= new Response();
            int &code_=rsp->code;
            conn->RecvOneLine(rq->rq_line);
            rq->RequestLineParse();
            if(!rq->IsMethodLegal())
            {
                conn->RecvRequestHead(rq->rq_head);
                code_=BAD_REQUEST;
                goto end;
            }
            rq->UriParse();
            if(!rq->IsPathLegal())
            {
                conn->RecvRequestHead(rq->rq_head);
                code_=NOT_FOUND;
                goto end;
            }
            LOG(INFO,"request path is OK!");
            conn->RecvRequestHead(rq->rq_head);
            if (rq->RequestHeadParse())
            {
                LOG(INFO,"parse head done!");
            }
            else
            {
                code_=BAD_REQUEST;
                goto end;
            }
            if(rq->IsNeedRecvText())
            {
                conn->RecvRequestText(rq->rq_text,rq->GetContentLength(),rq->GetParam());
            }
            ProcessResponse(conn,rq,rsp);

end:
    if(code_!=OK)
    {
        std::cout<<"into end"<<std::endl;
        HanderError(rq,rsp,conn);
    }

    delete conn;
    delete rq;
    delete rsp;
        }
};
#endif
