#pragma once

#include "thread_queue.h"
#include "task_msg.h"

class thread_pool
{
public:
    thread_pool(int thread_cnt);

    // 提供 获取 一个thread_queue的方法
    thread_queue<task_msg> * get_thread();

    // 发送 NEW_TASK 类型 task的任务接口
    // 发送一个task任务给thread_pool里的全部thread
    void send_task(task_func func, void * args=nullptr);
private:
    // 当前 thread_queue集合
    thread_queue<task_msg> ** _queues;

    // 线程个数
    int _thread_cnt;

    // 线程ID
    pthread_t * _tids;

    // 获取 线程的索引
    int _index;
};