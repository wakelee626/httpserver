#include"threadpool.hpp"
#include<unistd.h>
int print(int num)
{
    std::cout<<num<<std::endl;
}
int main()
{
    ThreadPool *tp=new ThreadPool();
    tp->initThreadPool();
    int i=0;
    for(;i<100000;i++)
    {
        Task t;
        t.SetTask(i,print);
        tp->PushTask(t);
        sleep(1);
    }
}
