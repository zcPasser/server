#pragma once

#include <netinet/in.h>
#include "event_loop.h"
#include "tcp_conn.h"
#include "message.h"
#include "thread_pool.h"

class tcp_server
{
public:
    //构造函数
    tcp_server(event_loop * loop, const char*ip, uint16_t port);
    //开始提供 创建连接的服务
    void do_accept();

    //析构函数 资源释放
    ~tcp_server();
private:
    //套接字 lfd
    int _sockfd;
    // 客户端连接地址
    struct sockaddr_in _connaddr;
    // 客户端连接地址 长度
    socklen_t _addrlen;

    // event_loop epoll的多路IO复用机制
    event_loop* _loop;

public:
    // 在线连接集合
    static tcp_conn * * conns;
    // 操作
    static void add_conn(int connfd, tcp_conn* conn);
    static void delete_conn(int connfd);
    static void get_conn_nums(int * curr_conn);

    static pthread_mutex_t _conns_mutex;
// 从配置文件中读的 TODO
#define MAX_CONNS 3
    // 当前允许连接 的 最大个数
    static int _max_conns;
    // 当前连接 刻度
    static int _curr_conns;

    // 路由分发机制 句柄
    static msg_router router;
    // 添加 路由的 方法
    void add_msg_router(int msgid, msg_callback cb, void *user_data=nullptr)
    {
        router.register_msg_router(msgid, cb, user_data);
    }
    
    // 设置 连接创建之后 KOOK函数 API
    static void set_conn_start(conn_callback cb, void * args=nullptr)
    {
        conn_start_cb = cb;
        conn_start_cb_args = args;
    }

    // 设置 连接销毁之前 KOOK函数 API
    static void set_conn_close(conn_callback cb, void * args=nullptr)
    {
        conn_close_cb = cb;
        conn_close_cb_args = args;        
    }

    // 创建连接之后，触发的回调函数
    static conn_callback conn_start_cb;
    static void* conn_start_cb_args;

    // 销毁连接之前，触发的回调函数
    static conn_callback conn_close_cb;
    static void* conn_close_cb_args;

    thread_pool* get_thread_pool()
    {
        return _thread_pool;
    }
private:
    // ========= 连接池 =========
    thread_pool * _thread_pool;
};

