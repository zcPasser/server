#include "tcp_conn.h"
#include "tcp_server.h"
#include "message.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

// 回显业务 回调函数
// 服务器写死的 回显业务
void callback_busi(const char*data, uint32_t len, int msgid, tcp_conn*conn, void*args){
    // tcp_conn 主动发送消息
    conn->send_message(data, len, msgid);
}

void conn_rd_callback(event_loop*loop, int fd, void*args){
    tcp_conn*conn=(tcp_conn*)args;
    conn->do_read();
}

void conn_wt_callback(event_loop*loop, int fd, void*args){
    tcp_conn*conn=(tcp_conn*)args;
    conn->do_write();
}

tcp_conn::tcp_conn(int connfd, event_loop * loop): _connfd(connfd), _loop(loop){
    // printf("function tcp_conn::tcp_conn:\n");
    // 1-将 _connfd 设置 非阻塞 状态
    // 将 _connfd 状态清空
    int flag = fcntl(_connfd, F_SETFL, 0);
    // 设置 非阻塞 状态
    fcntl(_connfd, F_SETFL, O_NONBLOCK | flag);

    // 2-设置 TCP_NODELAY 状态，禁止读写缓存，降低 小包延迟。
    // 不会出现 写缓存中累积到一定量之后，才会被发送出去 的现象，降低 小包延迟，但是 网络带宽利用率下降。
    int op = 1;
    setsockopt(_connfd, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));

    // 3-连接创建成功后，要触发的HOOK函数
    if(tcp_server::conn_start_cb != nullptr)
    {
        tcp_server::conn_start_cb(this, tcp_server::conn_start_cb_args);
    }
    // 4-将 当前 tcp_conn 的 读事件 加入到 loop中进行监听
    _loop->add_io_event(connfd, conn_rd_callback, EPOLLIN, this);

    // 5-将 自身 添加到 tcp_server::conns中管理
    tcp_server::add_conn(_connfd, this);
}

// 被动 处理 读业务 方法，由 event_loop 监听触发
void tcp_conn::do_read(){
    // 1-从 connfd 读数据
    int ret = _ibuf.read_data(_connfd);
    if(ret == -1){
        fprintf(stderr, "ibuf read_data error\n");
        this->clean_conn();
        return;
    }
    else if(ret == 0){
        // 对端客户端 正常 关闭
        printf("tcp_conn::tcp_conn: peer client has been closed!\n");
        this->clean_conn();        
    }

    msg_head head;

    // 2-读到 足够数据，是否满足 8字节 while
    while(_ibuf.length() >= MESSAGE_HEAD_LEN){
        // 2.1-先读 头部，得到 msgid,msglen
        memcpy(&head, _ibuf.data(), MESSAGE_HEAD_LEN);
        if(head.msglen > MESSAGE_LENGTH_LIMIT || head.msglen < 0){
            fprintf(stderr, "data format error, too large or too small,need close.\n");
            this->clean_conn(); 
            break;
        }

        // 2.2-再根据msglen 再进行一次 从 connfd读
        // 判断 得到的 消息体长度 ?= 头部里的长度
        if(_ibuf.length() < MESSAGE_HEAD_LEN + head.msglen){
            // 缓存中 buf 剩余的 收数据 小于 应该收到的数据
            // 说明 当前不是 一个 完整的包
            break;
        }
        // 表示当前包是合法的
        _ibuf.pop(MESSAGE_HEAD_LEN);

        // 处理 数据
        // printf("tcp_conn::do_read: read data = %s\n", _ibuf.data());
        // 固定的 连接业务（回显）
        // callback_busi(_ibuf.data(), head.msglen, head.msgid, this, nullptr);
        
        // 针对 不同msgid 调用 不同业务
        tcp_server::router.call(head.msgid, head.msglen, _ibuf.data(), this);

        // 数据处理结束
        _ibuf.pop(head.msglen);
    }

    _ibuf.adjust();

    return ;
}

// 被动 处理 写业务 方法，由 event_loop 监听触发
void tcp_conn::do_write(){
    //do_write是触发玩event事件要处理的事情，
    //应该是直接将out_buf力度数据io写会对方客户端 
    //而不是在这里组装一个message再发
    //组装message的过程应该是主动调用
    
    //只要obuf中有数据就写
    while (_obuf.length()) {
        int ret = _obuf.write2fd(_connfd);
        if (ret == -1) {
            fprintf(stderr, "tcp_conn::do_write: write2fd error, close conn!\n");
            this->clean_conn();
            return ;
        }
        if (ret == 0) {
            //不是错误，仅返回0表示不可继续写
            break;
        }
    }

    if (_obuf.length() == 0) {
        //数据已经全部写完，将_connfd的写事件取消掉
        _loop->del_io_event(_connfd, EPOLLOUT);
    }

    return ;
}

// 主动发送消息 方法
int tcp_conn::send_message(const char * data, int msglen, int msgid){
    printf("tcp_conn::send_message: data=%s, msglen=%d, msgid=%d\n", data, msglen, msgid);
    // 0- 监听 写事件，回调，只有当前数据发完了，才可激活。 
    bool active_epollout = false; 
    if(_obuf.length() == 0) {
        //如果现在已经数据都发送完了，那么是一定要激活写事件的
        //如果有数据，说明数据还没有完全写完到对端，那么没必要再激活等写完再激活
        active_epollout = true;
    }
    // 写到 _obuf
    // 1-封装 message消息头
    msg_head head;
    head.msgid = msgid;
    head.msglen = msglen;
    // 1.1 写消息头
    int ret = _obuf.send_data((const char *)&head, MESSAGE_HEAD_LEN);
    if (ret != 0) {
        fprintf(stderr, "tcp_conn::send_message: send head error\n");
        return -1;
    }
    // 2-写 消息体
    ret = _obuf.send_data(data, msglen);
    if (ret != 0) {
        //如果写消息体失败，那就回滚将消息头的发送也取消
        _obuf.pop(MESSAGE_HEAD_LEN);
        return -1;
    }
    // 3-将 _connfd 添加 写事件EPOLLOUT 和 回调，让 回调 负责 直接将 _obuf数据 发送给 对端客户端
    if (active_epollout == true) {
        _loop->add_io_event(_connfd, conn_wt_callback, EPOLLOUT, this);
    }

    return 0;
}

// 销毁 当前 连接
void tcp_conn::clean_conn(){
    // 0-连接销毁之前，要触发的HOOK函数
    if(tcp_server::conn_close_cb != nullptr)
    {
        tcp_server::conn_close_cb(this, tcp_server::conn_start_cb_args);
    }
    //链接清理工作
    //1 将该链接从tcp_server摘除掉    
    tcp_server::delete_conn(_connfd);
    //2 将该链接从event_loop中摘除
    _loop->del_io_event(_connfd);
    //3 buf清空
    _ibuf.clear(); 
    _obuf.clear();
    //4 关闭原始套接字
    int fd = _connfd;
    _connfd = -1;
    close(fd);
}