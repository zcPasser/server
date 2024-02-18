#pragma once

#include "net_connection.h"
#include "event_loop.h"
#include "reactor_buf.h"

class tcp_conn: public net_connection
{
public:
    tcp_conn(int connfd, event_loop * loop);
    // 被动 处理 读业务 方法，由 event_loop 监听触发
    void do_read();
    // 被动 处理 写业务 方法，由 event_loop 监听触发
    void do_write();
    // 主动发送消息 方法
    virtual int send_message(const char * data, int msglen, int msgid);
    virtual int get_fd()
    {
        return this->_connfd;
    }
    // 销毁 当前 连接
    void clean_conn();
private:
    int _connfd;
    // 当前连接 归属的 which event_loop
    event_loop * _loop;
    // 输出 outbuf
    output_buf _obuf;
    // 输入 inputbuf
    input_buf _ibuf;
};