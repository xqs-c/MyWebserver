#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;//前向声明

//用户数据结构
struct client_data
{
    sockaddr_in address; //客户端socket地址
    int sockfd; //socket文件描述符
    util_timer *timer;//定时器
};

//定时器类
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire; //任务超时时间
    
    void (* cb_func)(client_data *); //任务回调函数
    client_data *user_data;//指向用户数据的指针
    util_timer *prev;//指向前一个定时器
    util_timer *next;//指向后一个定时器
};

//定时器链表。一个升序，双向链表，带有头尾节点
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);//将目标定时器添加到链表中
    void adjust_timer(util_timer *timer);//当某个定时任务发生变化时，调整对应的定时器在链表中的位置
    void del_timer(util_timer *timer);//将目标定时器从链表中删除
    void tick();//SIGALRM信号每次被触发就在其信号处理函数中执行一次tick函数，以处理链表上到期的任务

private:
    void add_timer(util_timer *timer, util_timer *lst_head);//重载的辅助函数，将目标定时器timer添加到节点lst_head之后的部分链表中

    util_timer *head;
    util_timer *tail;
};

//管理定时器类
class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;//指向管道文件描述符的指针
    sort_timer_lst m_timer_lst; //升序定时器链表
    static int u_epollfd;//指向epoll文件描述符
    int m_TIMESLOT; //最小超时单位
};

void cb_func(client_data *user_data);//任务回调函数

#endif
