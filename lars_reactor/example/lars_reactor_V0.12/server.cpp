#include "tcp_server.h"
#include "config_file.h"
// #include "net_connection.h"

#include <string.h>

// ========= 辅助变量 =========
// 全局 server句柄
tcp_server * server;

// 玩家
struct player
{
    // 可以跟 对应的客户端玩家进行通信
    net_connection * conn;
    int player_id;
};
// ========= end =========

// ========= 辅助函数 =========
// ======= 回显业务 =======
// typedef void (*msg_callback)(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data);
void callback_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    // 直接 将数据发回去
    // net_connection * conn->tcp_conn对象，即 能和对端客户端 通信的 tcp_conn连接对象
    conn->send_message(data, len, msgid);

    printf("conn params=[%s]\n", (char*)conn->params);
}
// 打印业务
void print_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    printf("print busi is called!!!\n");

    printf("recv from client: [%s]\n", data);
    printf("msgid = %d\n", msgid);
    printf("len = %d\n", len);

    // 判断 是哪一个玩家所发送的消息
    struct player* new_player = (struct player*)conn->params;

}
// ======= end =======

// ======= 回调函数 =======
void print_lars_task(event_loop* loop, void * args)
{
    printf("=====> Activate Task Func <=====\n");

    // 获取 当前thread 在线 fd
    listen_fd_set fds;
    // 每个线程 监听的fd 是不同的。
    loop->get_listen_fds(fds);

    listen_fd_set::iterator it;
    for(it = fds.begin(); it != fds.end(); it++)
    {
        // 当前线程 已经建立连接 并且 监听中的 在线客户端
        int fd = *it;
        // 取出连接
        tcp_conn * conn = tcp_server::conns[fd];

        if(conn != nullptr)
        {
            int msgid = 404;
            const char* msg = "Hello, i am a Task!";
            conn->send_message(msg, strlen(msg), msgid);
        }
    }
}
// ======= end =======

// ======= hook钩子 =======
// 新客户端创建成功后 回调
void on_client_build(net_connection * conn, void*args)
{
    int msgid = 200;
    const char*msg = "Welcome! You are online!!!";

    conn->send_message(msg, strlen(msg), msgid);

    // 每次客户端 创建连接成功之后，执行一个任务
    server->get_thread_pool()->send_task(print_lars_task);

    // 创建 新玩家对象
    struct player* new_player = new player();
    new_player->conn = conn;
    // 累加器 累加得到
    int player_id = 1;
    new_player->player_id = player_id;

    conn->params = new_player;

    // 给当前conn 绑定 自定义参数，供后来使用
    const char* conn_param_test = "I am the conn param for you!";
    conn->params = (void*)conn_param_test;
}
// 客户端断开之前 回调
void on_client_lost(net_connection * conn, void*args)
{
    printf("connection is lost\n");
}
// ======= end =======


// ========= end =========

int main()
{
    event_loop loop;

    // 加载 配置文件
    config_file::set_path("./reactor.conf");
    std::string ip = config_file::instance()->get_string("reactor", "ip", "0.0.0.0");
    short port = config_file::instance()->get_number("reactor", "port", 8888);

    server = new tcp_server(&loop, ip.c_str(), port);

    // 注册 回调方法
    server->add_msg_router(1, callback_busi);
    server->add_msg_router(2, print_busi);

    // 注册 连接 HOOK函数
    server->set_conn_start(on_client_build);
    server->set_conn_close(on_client_lost);

    loop.event_process();

    return 0;
}