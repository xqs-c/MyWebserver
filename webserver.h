#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    /*服务器初始化函数
    port：端口号,默认9006
    user：登录名
    passWord：登陆密码
    databaseName：数据库名
    log_write：日志写入方式，默认同步
    opt_linger：优雅关闭链接，默认不使用
    trigmode：触发组合模式，默认listenfd LT + connfd LT
    sql_num：数据库连接池数量,默认8
    thread_num：线程池内的线程数量,默认8
    close_log：关闭日志,默认不关闭
    actor_model：并发模型,默认是proactor
    */
    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();//初始化线程池
    void sql_pool();//初始化数据库连接池
    void log_write();//初始化日志系统
    void trig_mode();//设置触发器模式
    void eventListen();//监听
    void eventLoop();//循环
    void timer(int connfd, struct sockaddr_in client_address);//根据用户socket地址和fd建立一个定时器
    void adjust_timer(util_timer *timer);//调整定时器
    void deal_timer(util_timer *timer, int sockfd);//销毁定时器，关闭连接
    bool dealclinetdata();//处理用户数据，为每一个客户连接建立定时器
    bool dealwithsignal(bool& timeout, bool& stop_server);//处理信号
    void dealwithread(int sockfd);//处理读数据事件
    void dealwithwrite(int sockfd);//处理写数据事件

public:
    //基础
    int m_port;          //端口号
    char *m_root;        //指向root用户的指针，指向当前文件夹的root文件的路径字符串
    int m_log_write; //写日志的模式
    int m_close_log;//是否关闭日志
    int m_actormodel;//事件处理模式：reactor/proactor

    int m_pipefd[2];//一个全双工管道
    int m_epollfd;//epoll文件描述符，用来唯一标识内核事件表
    http_conn *users;//指向一个http连接任务数组的指针

    //数据库相关
    connection_pool *m_connPool;//指向数据库连接线程池的指针
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;         //数据库连接池数量

    //线程池相关
    threadpool<http_conn> *m_pool;//指向线程池的指针，线程池中的线程用来处理http连接的任务
    int m_thread_num;//线程池中线程的数量

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER]; //epoll监测到的就绪事件的数组

    int m_listenfd;//监听socket文件描述符
    int m_OPT_LINGER;//优雅关闭链接
    int m_TRIGMode;//触发器模式
    int m_LISTENTrigmode;//监听触发器模式
    int m_CONNTrigmode;//连接触发器模式

    //定时器相关
    client_data *users_timer;//客户数据结构数组
    Utils utils;//定时器管理类
};
#endif
