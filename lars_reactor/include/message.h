#pragma once

#include "tcp_conn.h"
#include "net_connection.h"
#include <ext/hash_map>
#include <iostream>

// OsX1RImlOKmQKzvP

// 解决 tcp 粘包问题的 消息封装头
struct msg_head{
    // 当前消息类型
    int msgid;
    // 消息头长度
    int msglen;
};

// 消息头长度，固定值
#define MESSAGE_HEAD_LEN 8
// 消息头 + 消息体 最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

//===================== 消息分发路由机制 ==================
class tcp_client;
typedef void (*msg_callback)(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data);


class msg_router
{
public:
    msg_router(): _router(), _args()
    {
        printf("msg_router init ...\n");
    }

    int register_msg_router(int msgid, msg_callback msg_cb, void *user_data)
    {
        if(_router.find(msgid) != _router.end())
        {
            std::cout << "msgid " << msgid << "is already registered ..." << std::endl;
            return -1;
        }
        std::cout << "add msg callback msgID = " << msgid << std::endl;
        _router[msgid] = msg_cb;
        _args[msgid] = user_data;

        return 0;
    }

    void call(int msgid, uint32_t msglen, const char *data, net_connection *conn)
    {
        // std::cout << "call msgid = " << msgid << std::endl;

        if(_router.find(msgid) == _router.end())
        {
            std::cout << "msgid = " << msgid << " is not registered" << std::endl;
            return ;
        }

        msg_callback msg_cb = _router[msgid];
        void * user_data = _args[msgid];

        msg_cb(data, msglen, msgid, conn, user_data);
    }
private:
    // 针对 消息的 路由分发，key：msgid，value：注册的回调业务函数
    __gnu_cxx::hash_map<int, msg_callback> _router;
    // 路由业务函数 对应的 形参
    __gnu_cxx::hash_map<int, void*> _args;
};
