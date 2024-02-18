#include "udp_client.h"
#include <string.h>

// 注册 客户端 处理服务器返回数据 的 回调业务
// typedef void (*msg_callback)(const char*data, uint32_t len, int msgid, tcp_client*client, void*user_data);

void print_callback(const char*data, uint32_t len, int msgid, net_connection*conn, void*user_data)
{
    printf("recv from server\n");
    printf("msgid = %d, len = %d\n", msgid, len);
    printf("data = %s\n", data);
    printf("==================================\n");
}


int main()
{
    event_loop loop;
    udp_client client(&loop, "127.0.0.1", 7777);
    // 注册 回调业务
    // client.set_msg_callback(busi_callback);
    client.add_msg_router(1, print_callback);

    // udp client 主动发送消息给服务器
    int msgid = 1;
    const char * msg = "Hello, I am Lars udp client!";

    client.send_message(msg, strlen(msg), msgid);

    loop.event_process();

    return 0;
}