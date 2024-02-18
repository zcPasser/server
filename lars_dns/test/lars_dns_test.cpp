#include "lars_reactor.h"
// #include "dns_route.h"
#include "lars.pb.h"

// ========= callback =========
// 处理 dns 回复的消息
void deal_get_route(const char*data, uint32_t len, int msgid, net_connection* conn, void* user_data)
{
    //解包得到数据
    lars::GetRouteResponse rsp;
    rsp.ParseFromArray(data, len);
    
    //打印数据
    printf("modid = %d\n", rsp.modid());
    printf("cmdid = %d\n", rsp.cmdid());
    printf("host_size = %d\n", rsp.host_size());

    for (int i = 0; i < rsp.host_size(); i++) {
        printf("-->ip = %u\n", rsp.host(i).ip());
        printf("-->port = %d\n", rsp.host(i).port());
    }
}

// 主动给dns 发送请求 host主机集合消息
void on_connection(net_connection* conn, void* args)
{
    //发送Route信息请求
    lars::GetRouteRequest req;

    req.set_modid(1);
    req.set_cmdid(2);

    std::string requestString;

    req.SerializeToString(&requestString);
    conn->send_message(requestString.c_str(), requestString.size(), lars::ID_GetRouteRequest);
}

int main(int argc, char **argv)
{
    event_loop loop;

    //创建tcp
    tcp_client *client = new tcp_client(&loop, "127.0.0.1", 7778, "client_test1");
    if (client == nullptr) {
        fprintf(stderr, "client == NULL\n");
        exit(1);
    }
    client->set_conn_start(on_connection);

    // 注册 回调业务
    client->add_msg_router(lars::ID_GetRouteResponse, deal_get_route);

    loop.event_process();

    return 0;
}