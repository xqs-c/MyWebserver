# MyWebserver
该项目主要是在Linux环境下使用C++语言开发的轻量级多线程HTTP服务器，服务器支持一定数量的客户端连接并及时响应，支持 客户端访问服务器的图片、视频等资源。

* 使用**Socket**实现不同主机之间的网络通信
* 使用**Epoll**技术实现I/O多路复用，提高通信效率
* 使用**有限状态机**解析HTTP请求报文，对**GET**和**POST**请求进行处理
* 访问服务器数据库实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**
* 实现了**Reactor**和**模拟Proactor**两种事件处理模式，实现了**ET/LT**两种触发模式
* 利用**多线程**的机制，增加并行服务数量，提高服务器效率
