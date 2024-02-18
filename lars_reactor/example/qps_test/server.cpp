#include "tcp_server.h"
#include "config_file.h"
#include "echoMessage.pb.h"
// #include "net_connection.h"

#include <string.h>

// 定义 回显业务
// typedef void (*msg_callback)(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data);
void callback_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    qps_test::EchoMessage request, response;

    // 解包，将 data中的 proto协议 解析出来 放入request
    request.ParseFromArray(data, len);

    // printf("content = %s\n", request.content());
    // printf("id = %d\n", request.id());

    // 回显业务 制作一个新的proto数据包 EchoMessage类型的包  


    // 赋值
    response.set_id(request.id());
    response.set_content(request.content());

    // 将 response 序列化
    std::string responseString;
    response.SerializeToString(&responseString);
    
    // 将 回执的message 发送给 客户端
    conn->send_message(responseString.c_str(), responseString.size(), msgid);
}


int main()
{
    event_loop loop;

    // 加载 配置文件
    config_file::set_path("./reactor.conf");
    std::string ip = config_file::instance()->get_string("reactor", "ip", "0.0.0.0");
    short port = config_file::instance()->get_number("reactor", "port", 8888);
    
    printf("ip = %s, port = %d\n", ip.c_str(), port);
    
    tcp_server server(&loop, ip.c_str(), port);

    // 注册 回调方法
    server.add_msg_router(1, callback_busi);

    loop.event_process();

    return 0;
}