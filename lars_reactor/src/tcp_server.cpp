#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

#include "tcp_server.h"
#include "reactor_buf.h"
#include "tcp_conn.h"
#include "config_file.h"


#if 0
void server_rd_callback(event_loop*loop, int fd, void*args);
void server_wt_callback(event_loop*loop, int fd, void*args);
#endif

// ========= 针对 连接管理 =========
// 在线集合 及其 操作
tcp_conn ** tcp_server::conns = nullptr;
int tcp_server::_curr_conns = 0;
int tcp_server::_max_conns = 0;

// ========= 初始化 路由分发机制句柄 =========
msg_router tcp_server::router;

// ========= 初始化 HOOK函数 =========
conn_callback tcp_server::conn_start_cb = nullptr;
conn_callback tcp_server::conn_close_cb = nullptr;
void * tcp_server::conn_start_cb_args = nullptr;
void * tcp_server::conn_close_cb_args = nullptr;

pthread_mutex_t tcp_server::_conns_mutex = PTHREAD_MUTEX_INITIALIZER;

void tcp_server::add_conn(int connfd, tcp_conn* conn)
{
    pthread_mutex_lock(&_conns_mutex);
    conns[connfd] = conn;
    _curr_conns++;
    pthread_mutex_unlock(&_conns_mutex);
}

void tcp_server::delete_conn(int connfd)
{
    pthread_mutex_lock(&_conns_mutex);
    conns[connfd] = nullptr;
    _curr_conns--;
    pthread_mutex_unlock(&_conns_mutex);
}

void tcp_server::get_conn_nums(int * curr_conn)
{
    pthread_mutex_lock(&_conns_mutex);
    *curr_conn = _curr_conns;
    pthread_mutex_unlock(&_conns_mutex);
}
// ========= 以上针对 连接管理 =========

void lars_hello()
{
    std::cout << "lars Hello!" << std::endl;
}

#if 0
// 临时的收发消息结构
struct message{
    char data[m4K];
    int len;
};
struct message msg;

// connfd 注册的读事件的 回调业务
void server_wt_callback(event_loop*loop, int fd, void*args){
    struct message* msg=(struct message*)args;

    output_buf obuf;

    // 将msg --> obuf
    obuf.send_data(msg->data, msg->len);
    while(obuf.length()){
        int write_num = obuf.write2fd(fd);
        if(write_num == -1){
            fprintf(stderr, "obuf.write2fd error\n");
            return;
        }
        else if(write_num == 0){
            // 当前 不可写
            // 等待 下次触发
            break;
        }
    }
    // 删除 写事件，添加 读事件
    loop->del_io_event(fd, EPOLLOUT);
    loop->add_io_event(fd, server_rd_callback, EPOLLIN, msg);
}

// connfd 注册的读事件的 回调业务
void server_rd_callback(event_loop*loop, int fd, void*args){
    struct message* msg=(struct message*)args;
    int ret=0;

    input_buf ibuf;
    ret = ibuf.read_data(fd);
    if(ret == -1){
        fprintf(stderr, "ibuf.read_data error\n");
        // 当前读事件 删除
        loop->del_io_event(fd);

        // 关闭对端fd
        close(fd);
        return;
    }
     if(ret == 0){
        // 对方 正常关闭
        // 当前读事件 删除
        loop->del_io_event(fd);

        // 关闭对端fd
        close(fd);
        return;
    }
    std::cout << "server_rd_callback: server_rd_callback is called!" << std::endl;

    // 读取数据 拷贝到 msg
    msg->len = ibuf.length();
    bzero(msg->data, msg->len);
    memcpy(msg->data, ibuf.data(), msg->len);

    ibuf.pop(msg->len);
    ibuf.adjust();

    std::cout << "server_rd_callback: recv data =" << msg->data << std::endl;

    // 删除 读事件， 添加 写事件
    loop->del_io_event(fd, EPOLLIN);
    // epoll_wait 会立刻触发 
    loop->add_io_event(fd, server_wt_callback, EPOLLOUT, msg);
}
#endif

// IO事件触发的回调函数
// typedef void (*io_callback)(int fd, void *args);
void accept_callback(event_loop*loop, int fd, void*args){
    tcp_server*server = (tcp_server*)args;
    server->do_accept();
}

//构造函数
tcp_server::tcp_server(event_loop * loop, const char*ip, uint16_t port)
{
    // 优化：0
    // 0-忽略一些信号 SIGHUP SIGPIPE
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
    {
        // fprintf(stderr, "signal ignore SIGHUP failed\n");
        perror("signal ignore SIGHUP");
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        // fprintf(stderr, "signal ignore SIGPIPE failed\n");
        perror("signal ignore SIGPIPE");
    }
    // 1-创建socket
    // SOCK_CLOEXEC：连接关闭时，释放资源。
    _sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (_sockfd == -1)
    {
        fprintf(stderr, "tcp::server : socket()\n");
        exit(1);
    }
    // 2-初始化服务器地址
    struct sockaddr_in server_addr;
    // 清空
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);
    // 优化：2.5
    // 2.5-设置 socket 可以重复监听
    int reused_addr = 1;
    if(setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &reused_addr, sizeof(reused_addr)) < 0)
    {
        // fprintf(stderr, "setsockopt reused_addr failed\n");
        perror("setsockopt reused_addr");        
    }
    // 3-绑定端口
    if (bind(_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    // 4-监听
    if (listen(_sockfd, 500) == -1)
    {
        fprintf(stderr, "bind error\n");
        exit(1);
    }

    // 5-将形参loop添加到 tcp_server _loop
    _loop = loop;

    // 6-创建线程池
    // 从配置文件中读取-建议配置个数 和 用来网络通信CPU个数一致
    int thread_cnt = config_file::instance()->get_number("reactor", "threadNum", 3);
    if(thread_cnt > 0)
    {
        _thread_pool = new thread_pool(thread_cnt);
        if(_thread_pool == nullptr)
        {
            fprintf(stderr, "new thread_pool error\n");
            exit(1);
        }
    }
    // 7-连接管理
    // TODO 从配置文件 读取 最大个数
    _max_conns = config_file::instance()->get_number("reactor", "maxConn", 1000);
    // 预留 0、1、2 标准输入、输出、错误
    conns = new tcp_conn*[_max_conns + 5 + 2 * thread_cnt];
    // stdin stdout stderr
    // lfd
    // main_thread: epoll_fd
    // sub_thread: epoll_fd * thread_cnt
    // thread_queue: evfd * thread_cnt
    
    if(conns == nullptr)
    {
        fprintf(stderr, "new conns[%d] error\n", _max_conns);
        exit(1);
    }


    // 8-lfd 注册 _sockfd 读事件
    loop->add_io_event(_sockfd, accept_callback, EPOLLIN, this);

}

//开始提供 创建连接的服务
void tcp_server::do_accept()
{
    int connfd;
    // while(1)
    while(true)
    {
        // 1-accept
        _addrlen = 1;
        
        connfd = accept(_sockfd, (struct sockaddr*)&_connaddr, &_addrlen);
        if(connfd == -1)
        {
            // 中断错误，合法。
            if(errno == EINTR)
            {
                fprintf(stderr, "accept: errno == EINTR\n");
                continue;
            }
            // 资源不可用，须重试。比如出现在非阻塞操作中。
            else if(errno == EAGAIN)
            {
                fprintf(stderr, "accept: errno == EAGAIN\n");
                break;                
            }
            // 文件描述符值过大==建立连接过多，资源不够
            else if(errno == EMFILE)
            {
                fprintf(stderr, "accept: errno == EMFILE\n");
                continue;              
            }
            else
            {
                fprintf(stderr, "accept error\n");
                exit(1);              
            }
        }
        else
        {
            
            // accept succ
            // TODO 添加心跳机制
            // TODO 添加消息队列机制

            // accept succ!
            // 判断 连接个数 是否超过 最大值 _max_conns
            // 获取 连接刻度
            int cur_conns;
            get_conn_nums(&cur_conns);
            if(cur_conns >= _max_conns)
            {
                fprintf(stderr, "so many connections, max = %d\n", _max_conns);
                close(connfd);
            }
            else
            {                
                if(_thread_pool != nullptr)
                {
                    // 多线程模式
                    // 将 该connfd 交给一个线程去创建并监听
                    // 1-从线程池中获取一个thread_queue
                    thread_queue<task_msg>* que = _thread_pool -> get_thread();
                    // 组装 任务消息
                    task_msg task;
                    task.type = task_msg::NEW_CONN;
                    task.connfd = connfd;
                    // 2-将connfd发送到thread_queue中
                    que->send(task);
                }
                else
                {
                    // 单线程模式
                    // 创建 tcp_conn连接对象，将当前的连接用tcp_server所在线程来监听，属于单线程server
                    tcp_conn * conn = new tcp_conn(connfd, _loop);

                    if (conn == nullptr) {
                        fprintf(stderr, "tcp_server::do_accept: new tcp_conn error\n");
                        exit(1);
                    }
                }


                // ============================================
                std::cout<<"tcp_server::do_accept: accept succ!" << std::endl;
            }

            
            // 对点 客户端 已经 正常关闭。
            // close(connfd);
            break;
            // ======= example =======
            // int writed = 0;
            // const char* data = "hello Lars!\n";
            // do
            // {
            //     writed = write(connfd, data, strlen(data) + 1);
            // }while(writed == -1 && errno == EINTR);
            // // write succ
            // if(writed > 0)
            // {
            //     printf("write succ!\n");
            // }

        }
    }
}

//析构函数 资源释放
tcp_server::~tcp_server()
{
    close(_sockfd);
}