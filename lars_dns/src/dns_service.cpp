#include <ext/hash_set>
#include "lars_reactor.h"
#include "subscribe.h"
#include "dns_route.h"
#include "lars.pb.h"

tcp_server *server;

using __gnu_cxx::hash_set;

// agent客户端已经订阅的mod模块集合
typedef hash_set<uint64_t> client_sub_mod_list;

// 处理 agent 发送 Route 信息获取的业务
void get_route(const char *data, uint32_t len, int msgid, net_connection *net_conn, void *user_data)
{
    //1. 解析proto文件
    lars::GetRouteRequest req;

    req.ParseFromArray(data, len);

     
    //2. 得到modid 和 cmdid
    int modid, cmdid;

    modid = req.modid();
    cmdid = req.cmdid();
    
    //3. 根据modid/cmdid 获取 host信息
    host_set hosts = Route::instance()->get_hosts(modid, cmdid);

    //4. 将数据打包成protobuf
    lars::GetRouteResponse rsp;

    rsp.set_modid(modid);
    rsp.set_cmdid(cmdid);
    
    for (host_set_it it = hosts.begin(); it != hosts.end(); it ++) {
        uint64_t ip_port = *it;
        lars::HostInfo host;
        host.set_ip((uint32_t)(ip_port >> 32));
        host.set_port((int)(ip_port));
        rsp.add_host()->CopyFrom(host);
    }
    
    //5. 发送给客户端
    std::string responseString;
    rsp.SerializeToString(&responseString);
    net_conn->send_message(responseString.c_str(), responseString.size(), lars::ID_GetRouteResponse)    ;
}

//订阅route 的modid/cmdid
void create_subscribe(net_connection * conn, void *args)
{
    conn->params = new client_sub_mod_list;
}

//退订route 的modid/cmdid
void clear_subscribe(net_connection * conn, void *args)
{
    client_sub_mod_list::iterator it;
    client_sub_mod_list *sub_list = (client_sub_mod_list*)conn->params;

    for (it = sub_list->begin(); it  != sub_list->end(); it++) {
        uint64_t mod = *it;
        SubscribeList::instance()->unsubscribe(mod, conn->get_fd());
    }

    delete sub_list;

    conn->params = NULL;
}

int main(int argc, char **argv)
{
    event_loop loop;

    //加载配置文件
    config_file::set_path("conf/lars_dns.conf");
    std::string ip = config_file::instance()->get_string("reactor", "ip", "0.0.0.0");
    short port = config_file::instance()->get_number("reactor", "port", 7778);


    //创建tcp服务器
    server = new tcp_server(&loop, ip.c_str(), port);

    //==========注册链接创建/销毁Hook函数============
    server->set_conn_start(create_subscribe);
    server->set_conn_close(clear_subscribe);
  	//============================================

    //注册路由业务
    server->add_msg_router(lars::ID_GetRouteRequest, get_route);

    //开始事件监听    
    printf("lars dns service ....\n");
    loop.event_process();

    return 0;
}