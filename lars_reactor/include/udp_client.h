#pragma once

#include "event_loop.h"
#include "message.h"
#include "net_connection.h"

#include <string>
#include <netinet/in.h>

class udp_client: public net_connection
{
public:
    udp_client(event_loop* loop, const char *ip, uint16_t port);

    ~udp_client();

    // udp 主动发送消息
    virtual int send_message(const char * data, int msglen, int msgid);
    virtual int get_fd()
    {
        return this->_sockfd;
    }
    // 注册 一个msgid 和 回调业务 的路由关系
    void add_msg_router(int msgid, msg_callback cb, void * user_data=nullptr);

    void do_read();

private:
    int _sockfd;

    // stdin::string _name;

    char _read_buf[MESSAGE_LENGTH_LIMIT];
    char _write_buf[MESSAGE_LENGTH_LIMIT];

    event_loop* _loop;

    // 消息路由分发机制
    msg_router _router;


};