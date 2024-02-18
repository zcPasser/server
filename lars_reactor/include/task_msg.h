#pragma once



// thread_queue 消息队列 所能够 接收的 消息类型
struct task_msg
{
    // 2类 task 任务类型
    // 1- 建立连接 任务；
    // 2-普通任务（主线程 希望 主动分发 一些任务给 每个线程 来处理）
    enum TASK_TYPE
    {
        // 1- 建立连接 任务；
        NEW_CONN,
        // 2- 普通任务
        NEW_TASK
    };
    TASK_TYPE type;

    // 任务本身数据类型
    union
    {
        // 1-任务1，数据为 刚创建好的 connfd
        int connfd;
        // 2-任务2，数据为 具体数据参数和具体回调业务
        struct
        {
            void (*task_cb)(event_loop * loop, void * args);
            void * args;
        };
    };

};