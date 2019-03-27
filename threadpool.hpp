#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_
#include<iostream>
#include<queue>
#include<pthread.h>
#include"Log.hpp"
typedef int (*handler_t)(int);
class Task
{
    private: 
    int sock;
    handler_t handler;
    public:
    Task()
    {
        sock=-1;
        handler=NULL;
    }
    void SetTask(int sock_,handler_t handler_)
    {
        sock=sock_;
        handler=handler_;
    }
    void Run()
    {
        handler(sock); 
    }
    ~Task()
    {}  
};
#define NUM 5
class ThreadPool
{
  private:
        int thread_total_num;//一共有多少线程
        int thread_idle_num;//多少线程正在休眠
        std::queue<Task> task_queue;
        pthread_mutex_t lock;
        pthread_cond_t cond;//条件变量
        volatile bool is_quit;
        public:
        void LockQueue()
        {
            pthread_mutex_lock(&lock);
        }
        void UnlockQueue()
        {
            pthread_mutex_unlock(&lock);    
        }
        bool IsEmpty()
        {
            return task_queue.size()==0;
        }
        void ThreadIdle()
        {
            if(is_quit)
            {
                UnlockQueue();
                thread_total_num--;
                pthread_exit((void*)0);
                LOG(INFO,"thread exit...");
            }
            thread_idle_num++;
            pthread_cond_wait(&cond,&lock);
            thread_idle_num--;
        }
        void WakeupAllThread()
        {
            pthread_cond_broadcast(&cond);//唤醒全部线程
        }
        void WakeupOneThread()
        {
            pthread_cond_signal(&cond);
        }
       static void* thread_routine(void* arg)
        {
            ThreadPool *tp=(ThreadPool*)arg;
            pthread_detach(pthread_self());//不需要等待，分离此线程
            for(;;)
            {
              tp->LockQueue();
                while(tp->IsEmpty())
                {
                    tp->ThreadIdle();//没有任务等待   
                }
                Task t;
                tp->PopTask(t);
               tp-> UnlockQueue();
                LOG(INFO,"task has be taked,handler....");
                std::cout<<" thread id is"<<pthread_self()<<std::endl;
                t.Run();
            }
        }
        public:
        ThreadPool(int num_=NUM):thread_total_num(num_),thread_idle_num(0),is_quit(false)
        {
             pthread_mutex_init(&lock,NULL);
             pthread_cond_init(&cond,NULL);
        }
        void initThreadPool()
        {
            int i_=0;
            for(;i_<thread_total_num;i_++)
            {
                pthread_t tid;
                pthread_create(&tid,NULL,thread_routine,this);
            }
        }
        void PushTask(Task &t_)
        {
            LockQueue();
            if(is_quit)
            {
                UnlockQueue();
                return;
            }
            task_queue.push(t_);
            WakeupOneThread();
            UnlockQueue();
        }
        void PopTask(Task &t_)
        {
            t_=task_queue.front();
            task_queue.pop();
        }
        void Stop()
        {
            LockQueue();
            is_quit=true;
            UnlockQueue();
            while(thread_idle_num>0)
            {
                WakeupAllThread();
            }
        }
        ~ThreadPool()
        {
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&cond);
        }

};



#endif
