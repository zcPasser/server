#include "message.h"
#include "tcp_client.h"
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


void read_callback(event_loop* loop, int fd, void *args)
{
    // printf("function read_callback:\n");
    tcp_client*client=(tcp_client*)args;
    client->do_read();
}

void write_callback(event_loop*loop, int fd, void *args)
{
    // printf("function write_callback:\n");
    tcp_client*client=(tcp_client*)args;
    client->do_write();
}

// 若该函数被调用，则连接创建 成功
// 判断 链接是否已经判断成功，并且做出一定的创建成功之后的业务动作。
void connection_delay(event_loop*loop, int fd, void*args)
{
    // printf("function connection_delay:\n");
    tcp_client* client = (tcp_client*)args;
    //printf("connect %s:%d succ!\n", inet_ntoa(client->_server_addr.sin_addr), ntohs(client->_server_addr.sin_port));
    // 先 关闭 对 fd连接成功与否的 检测
    loop->del_io_event(fd);

    // 再次确认 连接
    int result = 0;
    socklen_t result_len = sizeof(result);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if(result == 0)
    {
        // 成功, fd中 没有任何错误。
        printf("connection succ!!\n");
        // 客户端业务
        if(client -> _conn_start_cb != nullptr)
            client -> _conn_start_cb(client, client -> _conn_start_cb_args);
        // // 主动发送消息 给服务端
        // const char*msg = "Hello lars, I am the client!";
        // int msgid = 1;
        // // 将 data 写到 当前client 的 _obuf
        // client->send_message(msg, strlen(msg), msgid);
        // 添加 针对 当前 client fd的 读回调
        loop->add_io_event(fd, read_callback, EPOLLIN, client);
        if(client->_obuf.length() > 0)
        {
            // 让 event_loop 触发 写回调 业务
            loop->add_io_event(fd, write_callback, EPOLLOUT, client);
        }
    }
    else
    {
        // 失败
        fprintf(stderr, "connect %s:%d error!\n", inet_ntoa(client->_server_addr.sin_addr), ntohs(client->_server_addr.sin_port));
        return ;
    }
    // 连接 创建 成功后的 业务

}

tcp_client::tcp_client(event_loop*loop, const char*ip, unsigned short port, const char* name): _router()
{
    // printf("function tcp_client::tcp_client:\n");
    _sockfd = -1;
    // _msg_callback = nullptr;
    _loop = loop;
    _name = name;
    // HOOK回调
    _conn_start_cb = nullptr;
    _conn_close_cb = nullptr;
    _conn_start_cb_args = nullptr;
    _conn_close_cb_args = nullptr;

    // 封装 即将要连接的 远程server端 IP地址
    bzero(&_server_addr, sizeof(_server_addr));

    _server_addr.sin_family = AF_INET;
    inet_aton(ip, &_server_addr.sin_addr);
    _server_addr.sin_port = htons(port);
    
    _addrlen = sizeof(_server_addr);

    // 连接 远程客户端
    this->do_connect();
}

// 发送方法
int tcp_client::send_message(const char*data, int msglen, int msgid)
{
    // printf("function tcp_client::send_message:\n");

    bool active_epollout = false;

    if(_obuf.length() == 0)
    {
        active_epollout = true;
    }
    // 1-封装 消息头
    msg_head head;
    head.msgid = msgid;
    head.msglen = msglen;

    // 2-将 消息头 写到 _obuf
    int ret = _obuf.send_data((const char*)&head, MESSAGE_HEAD_LEN);
    if(ret != 0)
    {
        fprintf(stderr, "send msg_head error\n");
        return -1;
    }
    // 3-写 消息体
    ret = _obuf.send_data(data, msglen);
    if(ret != 0)
    {
        fprintf(stderr, "send data error\n");
        // 若 消息体 写失败，消息头 需要 弹出重置
        _obuf.pop(MESSAGE_HEAD_LEN);
        return -1;
    }
    // 4-将 _sockfd 添加一个写事件 EPOLLOUT回调，让 回调直接将 数据写回 给对端
    if(active_epollout == true)
    {
        _loop->add_io_event(_sockfd, write_callback, EPOLLOUT, this);
    }
    return 0;
}

// 处理读业务
void tcp_client::do_read()
{
    // printf("function tcp_client::do_read:\n");
    // 1-从 _sockfd中 读数据
    int ret = _ibuf.read_data(_sockfd);
    if(ret == -1)
    {
        fprintf(stderr, "read data from _sockfd error\n");
        this->clean_conn();
        return ;
    }
    else if(ret == 0)
    {
        // 对端 _sockfd 正常 关闭
        printf("peer server closed\n");
        this->clean_conn();
        return;
    }
    // 读出 消息头
    msg_head head;
    // 2-读的数据 是否 满足8字节 while
    while(_ibuf.length() >= MESSAGE_HEAD_LEN)
    {
        // 2.1-先读 头部，得到 msgid 、msglen
        memcpy(&head, _ibuf.data(), MESSAGE_HEAD_LEN);
        if(head.msglen >MESSAGE_LENGTH_LIMIT || head.msglen < 0)
        {
            fprintf(stderr, "data format error, too large or too small, _sockfd needs to be closed\n");
            this->clean_conn();
            break;
        }
        // 2.2-判断得到的消息体长度 和 头部长度是否一致
        if(_ibuf.length() <MESSAGE_HEAD_LEN + head.msglen)
        {
            // 缓存中 buf剩余的收数据 < 应该收到的数据
            // 说明 当前 不是一个完整的包
            break;
        }
        // 表示当前包 合法
        _ibuf.pop(MESSAGE_HEAD_LEN);

#if 0
        // 处理——ibuf.data()业务数据
        // printf("name = %s, from %d, read data = %s", _name, _sockfd, _ibuf.data());
        // 执行 回显业务
        if(_msg_callback != nullptr)
        {
            this->_msg_callback(_ibuf.data(), head.msglen, head.msgid, this, nullptr);
        }
#endif

        // 消息路由分发机制来执行对应回调
        _router.call(head.msgid, head.msglen, _ibuf.data(), this);

        // 消息处理 结束
        _ibuf.pop(head.msglen);
    }
    _ibuf.adjust();
    return ;
}

// 处理写业务
void tcp_client::do_write()
{
    // printf("function tcp_client::do_write:\n");

    while(_obuf.length())
    {
        int write_num = _obuf.write2fd(_sockfd);
        if(write_num == -1)
        {
            fprintf(stderr, "tcp_client write _sockfd error\n");
            this->clean_conn();
            return;
        }
        else if(write_num == 0)
        {
            // 当前 不可写
            break;
        }
    }

    if(_obuf.length() == 0)
    {
        // 数据 全部写完，将 server对端的写事件 删除
        _loop->del_io_event(_sockfd, EPOLLOUT);
    }
    return ;
}

// 释放连接
void tcp_client::clean_conn()
{
    // printf("function tcp_client::clean_conn:\n");
    // HOOK钩子
    if(_conn_close_cb != nullptr)
        _conn_close_cb(this, _conn_close_cb_args);
    if(_sockfd != -1)
    {
        printf("del socket\n");
        _loop->del_io_event(_sockfd);
        close(_sockfd);
    }
    // 重新 发起 连接
    // this->do_connect();
}

// 连接服务器
void tcp_client::do_connect()
{
    // printf("function tcp_client::do_connect:\n");

    // 之前的连接
    if(_sockfd != -1)
        close(_sockfd);
    // 创建套接字
    _sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
    if(_sockfd == -1)
    {
        fprintf(stderr, "create tcp client socket error\n");
        exit(1);
    }

    int ret = connect(_sockfd, (const struct sockaddr *)&_server_addr, _addrlen);
    if(ret == 0)
    {
        // 创建 成功
        // 开发者 主动 手动调用 send_message 发包
        // 注册 读回调
        // _loop->add_io_event(_sockfd, read_callback, EPOLLIN, this);
        printf("connect %s:%d succ!\n", inet_ntoa(_server_addr.sin_addr), ntohs(_server_addr.sin_port));
        connection_delay(_loop, _sockfd, this);    
    }
    else
    {
        if(errno == EINPROGRESS)
        {
            // fd 为 非阻塞，并非失败
            // 若 fd 可写，则 创建成功
            printf("errno == EINPROGRESS\n");
            _loop->add_io_event(_sockfd, connection_delay, EPOLLOUT, this);
        }
        else
        {
            fprintf(stderr, "connect error\n");
            exit(1);
        }

    }
}

tcp_client::~tcp_client()
{
    close(_sockfd);
}