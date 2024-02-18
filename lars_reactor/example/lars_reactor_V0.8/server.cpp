#include "tcp_server.h"
// #include "net_connection.h"

#include <string.h>

// 定义 回显业务
// typedef void (*msg_callback)(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data);
void callback_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    printf("callback busi is called!!!\n");

    // 直接 将数据发回去
    // net_connection * conn->tcp_conn对象，即 能和对端客户端 通信的 tcp_conn连接对象
    conn->send_message(data, len, msgid);
}

// 打印业务
void print_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    printf("print busi is called!!!\n");

    printf("recv from client: [%s]\n", data);
    printf("msgid = %d\n", msgid);
    printf("len = %d\n", len);
}

// 新客户端创建成功后 回调
void on_client_build(net_connection * conn, void*args)
{
    int msgid = 200;
    const char*msg = "Welcome! You are online!!!";

    conn->send_message(msg, strlen(msg), msgid);
}

// 客户端断开之前 回调
void on_client_lost(net_connection * conn, void*args)
{
    printf("connection is lost\n");
}

int main()
{
    event_loop loop;

    tcp_server server(&loop, "127.0.0.1", 7777);

    // 注册 回调方法
    server.add_msg_router(1, callback_busi);
    server.add_msg_router(2, print_busi);

    // 注册 连接 HOOK函数
    server.set_conn_start(on_client_build);
    server.set_conn_close(on_client_lost);

    loop.event_process();

    return 0;
}