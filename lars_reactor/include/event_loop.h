#pragma once

#include "event_base.h"
#include <ext/hash_map>
#include <ext/hash_set>
#include <sys/epoll.h>
#include <vector>

#define MAXEVENTS 10

// ========= 辅助 =========
// map: key:fd ---> value:io_event
typedef __gnu_cxx::hash_map<int, io_event> io_event_map;
typedef __gnu_cxx::hash_map<int, io_event>::iterator io_event_map_it;

// set: value:fd 在线fd集合
typedef __gnu_cxx::hash_set<int> listen_fd_set;

// task 异步任务的回调函数类型
typedef void (*task_func)(event_loop * loop, void * args);

typedef std::pair<task_func, void*> task_func_pair;
// ========= 结束 =========


/*
 * 堆heap
 * */
class event_loop{
public:
    // 构造，创建 epoll
    event_loop();

    // 阻塞 循环监听事件， 并且处理
    void event_process();

    // 添加一个 IO事件 到 event_loop 中
    void add_io_event(int fd, io_callback proc, int mask, void* args=nullptr);

    // 删除一个 IO事件 从 event_loop 中
    void del_io_event(int fds);

    // 删除一个 IO事件 触发条件(EPOLLIN, EPOLLOUT, EPOLLIN | EPOLLOUT)
    void del_io_event(int fd, int mask);

    // ===> 针对 异步task任务 方法
    void add_task(task_func func, void *args);

    // 执行全部 _ready_tasks 任务
    void execute_ready_tasks();

    // 获取 全部监听事件的 fd集合
    void get_listen_fds(listen_fd_set & fds)
    {
        fds = listen_fds;
    }
private:
    // 通过 epoll_create创建
    int _epfd;
    // 当前event_loop 监控的 fd 和 对应事件的 关系
    io_event_map _io_evs;
    // 当前event_loop 正在监控 哪些fd， event_wait 等待哪些fd触发
    // 作用：当前服务器可以 主动发消息给客户端，当前fd集中——在线fd
    listen_fd_set listen_fds;
    
    // 每次 event_wait 返回触发，所返回的被激活的事件集合。
    struct epoll_event _fired_evs[MAXEVENTS];

    // ===> 针对 异步task任务属性
    // 目前已经就绪的所有task 队列
    std::vector<task_func_pair> _ready_tasks;
};
