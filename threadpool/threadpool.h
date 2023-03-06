#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>//多线程库
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

//线程池模板类
template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    connection_pool *m_connPool;  //数据库连接池
    int m_actor_model;          //模型切换
};

//构造函数
template <typename T>
threadpool<T>::threadpool( int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];//开辟线程池的空间
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        //创建成功应该返回0，如果线程池在线程创建阶段就失败，那就应该关闭线程池了
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) //创建线程，新线程将运行worker函数，其参数是this。为函数指针，指向处理线程函数的地址。该函数，要求为静态函数
        {
            delete[] m_threads;
            throw std::exception();
        }
        //主要是将线程属性更改为unjoinable，便于资源的释放
        //如果线程是joinable状态，当线程函数自己退出都不会释放线程所占用堆栈和线程描述符（总计8K多）。只有当主线程调用了pthread_join，主线程阻塞等待子线程结束，然后回收子线程资源。
        //unjoinable属性可以在pthread_create时指定，或在线程创建后在线程中pthread_detach（pthread_detach()即主线程与子线程分离，子线程结束后，资源自动回收）
        if (pthread_detach(m_threads[i])) //从状态上实现线程分离，注意不是指该线程独自占用地址空间
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

//线程池析构函数，释放申请的内存空间
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

//reactor模式下请求入队
template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();//上锁
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();//请求队列任务数大于最大值，解锁并返回加入队列失败
        return false;
    }
    request->m_state = state;//任务的状态
    m_workqueue.push_back(request);//将任务入队
    m_queuelocker.unlock();//解锁
    m_queuestat.post();//前操作，进行V操作
    return true;
}

//proactor模式下的请求入队
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

//工作线程:pthread_create时就调用了它
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    //调用时 *arg是this！
    //所以该操作其实是获取threadpool对象地址
    threadpool *pool = (threadpool *)arg;
    //线程池中每一个线程创建时都会调用run()，睡眠在队列中
    pool->run();
    return pool;
}

//线程池中的所有线程都睡眠，等待请求队列中新增任务
//每调用一次pthread_create就会调用一次run(),因为每个线程是相互独立的，都睡眠在工作队列上，仅当信号变量更新才会唤醒进行任务的竞争
template <typename T>
void threadpool<T>::run()
{
    while (true)
    {
        m_queuestat.wait();//后操作，P操作，等待请求队列插入新任务。若信号量为0，则阻塞
        m_queuelocker.lock();//加锁操作
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();//释放锁
            continue;
        }
        T *request = m_workqueue.front();//取出队头任务
        m_workqueue.pop_front();//弹出对头任务
        m_queuelocker.unlock();//解锁
        if (!request)
            continue;
        if (1 == m_actor_model)//reactor模式：工作线程先根据任务的状态确定对数据进行读还是写，然后在处理任务
        {
            if (0 == request->m_state)//读操作
            {
                if (request->read_once())
                {
                    request->improv = 1;//处理标志物位
                    connectionRAII mysqlcon(&request->mysql, m_connPool);//连接数据库
                    request->process();//处理任务
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else//写操作
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else//proactor模式：工作线程直接进行处理任务
        {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}
#endif
