#pragma once

// 抽象 连接类
class net_connection
{
public:
    net_connection(){}
    // 纯虚函数
    virtual int send_message(const char* data, int msglen, int msgid) = 0;
    virtual int get_fd() = 0;
    // 开发者 可以 通过该参数 传递一些动态的 自定义参数
    void * params;
};


// 创建连接之后 / 销毁连接之前，触发的回调函数 函数类型
typedef void(*conn_callback)(net_connection * conn, void * args);