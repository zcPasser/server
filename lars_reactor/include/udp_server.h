#pragma once

#include "event_loop.h"
#include "message.h"
#include "net_connection.h"

#include <netinet/in.h>


class udp_server: public net_connection
{
public:
    udp_server(event_loop* loop, const char* ip, uint16_t port);
    // udp 主动发送消息
    virtual int send_message(const char * data, int msglen, int msgid);
    virtual int get_fd()
    {
        return this->_sockfd;
    }
    // 注册 一个msgid 和 回调业务 的路由关系
    void add_msg_router(int msgid, msg_callback cb, void * user_data=nullptr);

    // 读业务
    void do_read();
    

    ~udp_server();

private:
    int _sockfd;

    event_loop* _loop;

    // 消息路由分发机制
    msg_router _router;

    char _read_buf[MESSAGE_LENGTH_LIMIT];
    char _write_buf[MESSAGE_LENGTH_LIMIT];

    // 客户端IP地址
    struct sockaddr_in _client_addr;
    socklen_t _client_addrlen;
};