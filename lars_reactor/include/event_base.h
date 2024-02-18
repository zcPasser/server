#pragma once

/*
 * 定义IO复用机制事件的封装(epoll原生事件来封装)
 *
 * */

class event_loop;

// IO事件触发的回调函数
typedef void (*io_callback)(event_loop* loop, int fd, void *args);

/*
 * 封装 一次IO触发 事件
 * */
struct io_event{
    io_event(): mask(0), read_callback(nullptr), write_callback(nullptr), rcb_args(nullptr), wcb_args(nullptr){}

    // 事件的 读写属性
    int mask; // EPOLLIN, EPOLLOUT, EPOLLIN | EPOLLOUT
    // 读事件触发所绑定的回调函数
    io_callback read_callback;
    // 写事件触发所绑定的回调函数
    io_callback write_callback;
    // 读事件回调函数 形参
    void* rcb_args;
    // 写事件回调函数 形参
    void* wcb_args;
};
