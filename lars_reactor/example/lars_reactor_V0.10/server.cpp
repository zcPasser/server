#include "udp_server.h"
#include "config_file.h"
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


int main()
{
    event_loop loop;

    // 加载 配置文件
    config_file::set_path("./reactor.conf");
    std::string ip = config_file::instance()->get_string("reactor", "ip", "0.0.0.0");
    short port = config_file::instance()->get_number("reactor", "port", 8888);

    udp_server server(&loop, ip.c_str(), port);

    // 注册 回调方法
    server.add_msg_router(1, callback_busi);
    // server.add_msg_router(2, print_busi);


    loop.event_process();

    return 0;
}