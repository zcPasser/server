#include "tcp_client.h"
#include <string.h>

// 注册 客户端 处理服务器返回数据 的 回调业务
// typedef void (*msg_callback)(const char*data, uint32_t len, int msgid, tcp_client*client, void*user_data);

void busi_callback(const char*data, uint32_t len, int msgid, net_connection*conn, void*user_data)
{
    // 写回 数据
    conn->send_message(data, len, msgid);
}

void print_callback(const char*data, uint32_t len, int msgid, net_connection*conn, void*user_data)
{
    printf("recv from server\n");
    printf("msgid = %d, len = %d\n", msgid, len);
    printf("data = %s\n", data);
    printf("==================================\n");
}

// 客户端创建连接之后的HOOK业务
void on_client_build(net_connection * conn, void *args)
{
    printf("function on_client_build:\n");
    int msgid = 1;
    const char * msg = "Hello Lars.";

    conn->send_message(msg, strlen(msg), msgid);
}

// 客户端销毁连接之前的HOOK业务
void on_client_lost(net_connection * conn, void *args)
{
    printf("function on_client_lost:\n");

}

int main()
{
    event_loop loop;
    tcp_client client(&loop, "127.0.0.1", 7777, "client 1");
    // 注册 回调业务
    // client.set_msg_callback(busi_callback);
    client.add_msg_router(1, print_callback);
    client.add_msg_router(2, busi_callback);
    client.add_msg_router(200, print_callback);
    client.add_msg_router(404, print_callback);

    // 注册 HOOK钩子
    client.set_conn_start(on_client_build);
    client.set_conn_close(on_client_lost);

    loop.event_process();
}