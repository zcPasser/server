#include "event_loop.h"
#include <iostream>

// 构造，创建 epoll
event_loop::event_loop(){
    // epoll句柄
    _epfd = epoll_create1(0);
    if(_epfd == -1){
        fprintf(stderr, "epoll_create1 error\n");
        exit(1);
    }
}

// 阻塞 循环监听事件， 并且处理 epoll_wait
void event_loop::event_process(){
    std::cout << "event_loop::event_process: wait I/O event.." << std::endl;
    io_event_map_it ev_it;
    while(true){
        int nfds = epoll_wait(_epfd, _fired_evs, MAXEVENTS, -1);
        for(int i=0; i < nfds; i++){
            ev_it = _io_evs.find(_fired_evs[i].data.fd);

            io_event * ev = &(ev_it->second);
            if(_fired_evs[i].events & EPOLLIN){
                void* args = ev->rcb_args;
                ev->read_callback(this, _fired_evs[i].data.fd, args);
            }
            else if(_fired_evs[i].events & EPOLLOUT){
                void * args = ev->wcb_args;
                ev->write_callback(this, _fired_evs[i].data.fd, args);
            }
            // 用于处理 epoll 事件中的 EPOLLHUP 或 EPOLLERR 事件，根据是否设置了读写回调函数来执行相应的操作。
            // 如果都没有设置回调函数，它将从 epoll 中删除这个事件，以确保它不再被监听。
            // 这种处理方式通常用于处理网络连接中可能出现的错误情况。
            else if(_fired_evs[i].events & (EPOLLHUP | EPOLLERR)){
                // 水平触发 未处理，可能会出现 HUP事件，需要正常处理读写，如果当前时间events 既没有写，也没有读，则将其从epoll删除
                if(ev->read_callback != nullptr){
                    void* args = ev->rcb_args;
                    ev->read_callback(this, _fired_evs[i].data.fd, args);                    
                }
                else if(ev->write_callback != nullptr){
                    void * args = ev->wcb_args;
                    ev->write_callback(this, _fired_evs[i].data.fd, args);                 
                }
                else{
                    // 删除
                    fprintf(stderr, "fd %d get error, delete it from epoll\n", _fired_evs[i].data.fd);
                    this->del_io_event(_fired_evs[i].data.fd);
                }
            }
        }
        // 当前 _epfd的全部 读写事件 执行完毕
        // 执行 其他task 任务流程 
        this->execute_ready_tasks();
    }
}

// 添加一个 IO事件 到 event_loop 中, 包含 add && mod
void event_loop::add_io_event(int fd, io_callback proc, int mask, void* args){
    int op;
    int final_mask;
    // 1-判断当前fd是否 为 已有事件
    // 若事件存在，以 MOD方式 添加到epoll
    io_event_map_it it = _io_evs.find(fd);
    if(it == _io_evs.end()){
        op = EPOLL_CTL_ADD;
        final_mask = mask;
    }
    else {
        op = EPOLL_CTL_MOD;
        final_mask = it->second.mask | mask;
    }

    // 2-fd io_callback 绑定到map
    if(mask & EPOLLIN){
        // 注册 读回调 业务
        _io_evs[fd].read_callback = proc;
        _io_evs[fd].rcb_args = args;
    }
    else if(mask & EPOLLOUT){
        // 注册 写回调 业务
        _io_evs[fd].write_callback = proc;
        _io_evs[fd].wcb_args = args;        
    }

    // 3-epoll_ctl添加到epoll堆里
    _io_evs[fd].mask = final_mask;

    // 4-将当前事件 加入到 原生epoll
    struct epoll_event event;
    event.events = final_mask;
    event.data.fd = fd;
    if(epoll_ctl(_epfd, op, fd, &event) == -1){
        fprintf(stderr, "epoll_ctl %d error\n", fd);
        return;
    }

    // 4-将当前fd 加入 在线监听的fd集合
    listen_fds.insert(fd);
}

// 删除一个 IO事件 从 event_loop 中
void event_loop::del_io_event(int fd){
    //如果没有该事件，直接返回
    io_event_map_it it = _io_evs.find(fd);
    if (it == _io_evs.end()) {
        return ;
    }
    _io_evs.erase(fd);

    listen_fds.erase(fd);

    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
}

// 删除一个 IO事件 触发条件(EPOLLIN, EPOLLOUT, EPOLLIN | EPOLLOUT)
void event_loop::del_io_event(int fd, int mask){
    //如果没有该事件，直接返回
    io_event_map_it it = _io_evs.find(fd);
    if (it == _io_evs.end()) {
        return ;
    }
    int &o_mask = it->second.mask;
    //修正mask
    o_mask = o_mask & (~mask);
    
    if (o_mask == 0) {
        //如果修正之后 mask为0，则删除
        this->del_io_event(fd);
    }
    else {
        //如果修正之后，mask非0，则修改
        struct epoll_event event;
        event.events = o_mask;
        event.data.fd = fd;
        epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &event);
    } 
}

// ===> 针对 异步task任务 方法
void event_loop::add_task(task_func func, void *args)
{
    task_func_pair func_pair(func, args);
    _ready_tasks.push_back(func_pair);
}

// 执行全部 _ready_tasks 任务
void event_loop::execute_ready_tasks()
{
    // 遍历 _ready_tasks 取出执行
    std::vector<task_func_pair>::iterator it;

    for(it = _ready_tasks.begin(); it != _ready_tasks.end(); it++)
    {
        // 取出 当前任务的执行函数
        task_func func = it->first;
        // 取出 当前任务的 参数
        void*args = it->second;

        func(this, args);
    }

    // 全部函数 执行完毕,情况当前 _ready_tasks
    _ready_tasks.clear();
}