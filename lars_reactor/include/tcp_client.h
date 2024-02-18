#pragma once
#include "net_connection.h"
#include "message.h"
#include "event_loop.h"
#include "reactor_buf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class tcp_client;

// typedef void (*msg_callback)(const char*data, uint32_t len, int msgid, tcp_client*client, void*user_data);

class tcp_client: public net_connection
{
public:
    tcp_client(event_loop*loop, const char*ip, unsigned short port, const char* name);
    // 发送方法
    virtual int send_message(const char*data, int msglen, int msgid);
    virtual int get_fd()
    {
        return this->_sockfd;
    }
    // 处理读业务
    void do_read();
    // 处理写业务
    void do_write();
    // 释放连接
    void clean_conn();
    // 连接服务器
    void do_connect();
#if 0
    // 设置 业务处理的 回调函数
    void set_msg_callback(msg_callback msg_cb)
    {
        this->_msg_callback = msg_cb;
    }
#endif    
    // 注册 消息路由回调函数
    void add_msg_router(int msgid, msg_callback cb, void * user_data=nullptr)
    {
        _router.register_msg_router(msgid, cb, user_data);
    }

    ~tcp_client();

    input_buf _ibuf;
    output_buf _obuf;
    // server端 IP
    struct sockaddr_in _server_addr;
    socklen_t _addrlen;

    // 设置 连接创建之后 KOOK函数 API
    void set_conn_start(conn_callback cb, void * args=nullptr)
    {
        _conn_start_cb = cb;
        _conn_start_cb_args = args;
    }

    // 设置 连接销毁之前 KOOK函数 API
    void set_conn_close(conn_callback cb, void * args=nullptr)
    {
        _conn_close_cb = cb;
        _conn_close_cb_args = args;        
    }

    // 创建连接之后 触发的 回调函数
    conn_callback _conn_start_cb;
    void * _conn_start_cb_args;
    // 销毁连接之前 触发的 回调函数
    conn_callback _conn_close_cb;
    void * _conn_close_cb_args;
private:
    int _sockfd;

    // 客户端 事件 处理机制
    event_loop* _loop;

#if 0
    // 处理 服务器消息的 回调业务函数
    msg_callback _msg_callback;
#endif

    const char* _name;
    // 路由分发机制 句柄
    msg_router _router;

};