#include "udp_server.h"

#include <signal.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


// ========= 辅助 =========
void read_callback(event_loop*loop, int fd, void*args)
{
    udp_server* server = (udp_server*)args;

    // 
    server->do_read();
}
// ========= =========


udp_server::udp_server(event_loop* loop, const char* ip, uint16_t port)
{
    // 0-忽略一些信号 SIGHUP SIGPIPE
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
    {
        // fprintf(stderr, "signal ignore SIGHUP failed\n");
        perror("signal ignore SIGHUP");
        exit(1);
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        // fprintf(stderr, "signal ignore SIGPIPE failed\n");
        perror("signal ignore SIGPIPE");
        exit(1);
    }

    // 1-创建socket
    // SOCK_CLOEXEC：连接关闭时，释放资源。
    _sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    if (_sockfd == -1)
    {
        perror("create udp_server error!");
        exit(1);
    }

    // 2-初始化服务器地址
    struct sockaddr_in server_addr;
    // 清空
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // 3-绑定端口
    if (bind(_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind error!");
        exit(1);
    }

    // 4-将形参loop添加到 udp_server _loop
    _loop = loop;

    // 5-清空 客戶端地址
    bzero(&_client_addr, sizeof(_client_addr));
    _client_addrlen = sizeof(_client_addr);

    // 6-监控_sockfd 读事件，如果 可读，标识 客户端有数据 传输过来。
    _loop->add_io_event(_sockfd, read_callback, EPOLLIN, this);

    printf("udp_server is running ip = %s, port=%d\n", ip, port);
}

// udp 主动发送消息
int udp_server::send_message(const char * data, int msglen, int msgid)
{
    // udp_server 主动发送消息
    if(msglen > MESSAGE_LENGTH_LIMIT)
    {
        fprintf(stderr, "message too large error.\n");
        return -1;        
    }

    // 组包
    msg_head head;
    head.msglen = msglen;
    head.msgid = msgid;

    // head 写到 输出缓存
    memcpy(_write_buf, &head, MESSAGE_HEAD_LEN);
    // data 写到 输出缓存
    memcpy(_write_buf + MESSAGE_HEAD_LEN, data, msglen);

    // by _client_addr 将报文发送给对端
    int ret = sendto(_sockfd, _write_buf, msglen + MESSAGE_HEAD_LEN, 0, (struct sockaddr *)&_client_addr, _client_addrlen);
    if(ret == -1)
    {
        perror("sendto error.");
        return -1;
    }
    return ret;
}

// 注册 一个msgid 和 回调业务 的路由关系
void udp_server::add_msg_router(int msgid, msg_callback cb, void * user_data)
{
    _router.register_msg_router(msgid, cb, user_data);
}

// 读业务
void udp_server::do_read()
{
    while(true)
    {
        int pkg_len = recvfrom(_sockfd, _read_buf, sizeof(_read_buf), 0, (struct sockaddr *)&_client_addr, &_client_addrlen);
        if(pkg_len == -1)
        {
            if(errno == EINTR)
            {
                // 中断错误
                continue;
            }
            else if(errno == EAGAIN)
            {
                // 非阻塞
                break;                
            }
            else
            {
                perror("udp_server recvfrom error!");
                break;
            }
        }
        // 数据来了
        msg_head head;
        // 获取 消息头部
        memcpy(&head, _read_buf, MESSAGE_HEAD_LEN);

        if(head.msglen > MESSAGE_LENGTH_LIMIT || head.msglen < 0 || head.msglen + MESSAGE_HEAD_LEN != pkg_len)
        {
            // 报文格式错误
            fprintf(stderr, "message head format error.\n");
            continue;
        }

        // 已获取 完整 message，根据msgid 执行 回调业务
        _router.call(head.msgid, head.msglen, _read_buf + MESSAGE_HEAD_LEN, this);
    }
}

udp_server::~udp_server()
{
    _loop->del_io_event(_sockfd);
    close(_sockfd);
}
