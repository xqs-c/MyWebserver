#include "config.h"//包含参数认证头文件

int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "debian-sys-maint";
    string passwd = "Of4tf0UmevEWVEcl";
    string databasename = "mydb";

    //命令行解析
    Config config;//创建一个认证类对象
    config.parse_arg(argc, argv);//调用认证类的成员函数完成命令行的解析

    //创建一个服务器对象：1、创建http_conn类对象 2、获取root文件夹路径 3、创建定时器对象
    WebServer server;  

    //初始化服务器，将命令行的参数传入服务器对象中
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);
    

    //日志系统
    server.log_write();

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //触发模式
    server.trig_mode();

    //监听：网络编程监听端口
    server.eventListen();

    //运行：利用epoll获取端口读写信息，若有读写事件，则工作线程调用对应的处理函数
    server.eventLoop();

    return 0;
}
