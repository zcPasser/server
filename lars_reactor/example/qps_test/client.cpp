#include "tcp_client.h"
#include "echoMessage.pb.h"
#include <string.h>
#include <pthread.h>
#include <ctime>

// ========= 辅助 =========
// qps记录句柄
struct Qps
{
    Qps()
    {
        last_time = time(0);
        succ_cnt = 0;
    }
    // 最后一次 发包时间，ms单位
    long last_time;
    // 成功收到服务器 回显业务的次数
    int succ_cnt;
};
// ========= 结束 =========

// 注册 客户端 处理服务器返回数据 的 回调业务
// typedef void (*msg_callback)(const char*data, uint32_t len, int msgid, tcp_client*client, void*user_data);

// 测试 qps性能
void qps_busi(const char*data, uint32_t len, int msgid, net_connection*conn, void*user_data)
{
    Qps * qps = (Qps*)user_data;

    qps_test::EchoMessage request, response;

    // 解包，将 data中的 proto协议 解析出来 放入request
    if(response.ParseFromArray(data, len) == false)
    {
        printf("server call back data error\n");
        return ;
    }

    if(response.content() == "QPS test!")
    {
        qps->succ_cnt ++;
    }

    long current_time = time(0);
    // printf("current_time=[%ld]\n", current_time);
    // printf("qps->last_time=[%ld]\n", qps->last_time);
    if(current_time - qps->last_time >= 1)
    {
        printf("QPS=[%d]\n", qps->succ_cnt);
        qps->succ_cnt = 0;
        qps->last_time = current_time;
    }

    // 给服务端发送新的请求
    request.set_id(response.id() + 1);
    request.set_content(response.content());

    // 将 request 序列化
    std::string requestString;
    request.SerializeToString(&requestString);
    // printf("qps.cnt=[%d], content=[%s]\n", qps->succ_cnt, requestString.c_str());
    // 将 回执的message 发送给 客户端
    conn->send_message(requestString.c_str(), requestString.size(), msgid);
}

// 客户端创建连接之后的HOOK业务
void on_client_build(net_connection * conn, void *args)
{
    qps_test::EchoMessage request;

    request.set_id(1);
    request.set_content("QPS test!");

    std::string requestString;

    request.SerializeToString(&requestString);
    
    // 给 server 发送消息
    int msgid = 1;
    conn->send_message(requestString.c_str(), requestString.size(), msgid);
}

// 客户端销毁连接之前的HOOK业务
void on_client_lost(net_connection * conn, void *args)
{
    // printf("function on_client_lost:\n");

}

// 多个客户端 模拟
void * thread_main(void*args)
{
    event_loop loop;
    tcp_client client(&loop, "127.0.0.1", 7777, "client 1");

    Qps qps;

    // 注册 回调业务
    client.add_msg_router(1, qps_busi, (void*)&qps);

    // 注册 HOOK钩子
    client.set_conn_start(on_client_build);
    // client.set_conn_close(on_client_lost);

    loop.event_process();
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        printf("Usage: ./client [threadNum]\n");
        return 1;
    }

    //创建N个线程
    int thread_num = atoi(argv[1]);
    pthread_t *tids;
    tids = new pthread_t[thread_num];

    for (int i = 0; i < thread_num; i++) {
        pthread_create(&tids[i], NULL, thread_main, NULL);
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}