#include "thread_pool.h"
#include "event_loop.h"
#include "tcp_conn.h"

#include <pthread.h>

// 有task任务消息过来，该业务函数 会被 loop监听到 并执行，读出消息队列里的消息并进行处理
void deal_task(event_loop* loop, int fd, void * args)
{
    // 1-从 thread_queue 取数据
    thread_queue<task_msg>* que = (thread_queue<task_msg>*)args;

    std::queue<task_msg> new_task_que;
    // new_task_que存放 thread_queue 全部任务
    que->recv(new_task_que);

    while(!new_task_que.empty())
    {
        task_msg task = new_task_que.front();

        new_task_que.pop();

        // 2-判断task类型
        if(task.type == task_msg::NEW_CONN)
        {
            // 任务1：获取的数据为刚刚accept成功的connfd
            // 让当前线程 去创建 连接，同时将连接的connfd 加入到当前的thread的event_loop中进行监听
            tcp_conn * conn = new tcp_conn(task.connfd, loop);
            if(conn == nullptr)
            {
                fprintf(stderr, "[thread]: new tcp_conn error!\n");
                exit(1);   
            }
            printf("[thread]: create new tcp_conn succ!\n");
        }
        else if(task.type == task_msg::NEW_TASK)
        {
            // 若是 任务2,普通任务
            // 该任务添加到event_loop 循环,执行
            loop->add_task(task.task_cb, task.args);
        }
        else
        {
            fprintf(stderr, "unknown task!\n");
            // exit(1);   
        }
    }

    // 2
}

void *thread_main(void * args)
{
    thread_queue<task_msg> * que = (thread_queue<task_msg>*) args;

    event_loop*loop = new event_loop();

    if(loop == nullptr)
    {
        fprintf(stderr, "new event_loop failed!\n");
        exit(1);        
    }

    que->set_loop(loop);
    que->set_callback(deal_task, que);

    // 启动 loop 监听
    // 此时 监听 queue.evfd、connfd
    loop->event_process();

    return nullptr;
}

// 每次 创建一个server ，就应该创建一个线程池
thread_pool::thread_pool(int thread_cnt)
{
    _queues = nullptr;
    _index = 0;
    _thread_cnt = thread_cnt;
    if(_thread_cnt <= 0)
    {
        fprintf(stderr, "thread_cnt > 0!\n");
        exit(1);
    }
    // 开辟 _thread_cnt个 thread_queue指针，每个指针并未 new对象
    _queues = new thread_queue<task_msg>*[_thread_cnt];
    _tids = new pthread_t[_thread_cnt];

    // 开辟线程
    int ret = 0;
    for(int i = 0; i < _thread_cnt; ++i)
    {
        // new
        _queues[i] = new thread_queue<task_msg>();
        
        // 绑定
        ret = pthread_create(&_tids[i], nullptr, thread_main, _queues[i]);
        if(ret == -1)
        {
            fprintf(stderr, "[thread %d] pthread_create failed!\n", i);
            exit(1);
        }

        // 线程设置 detach 模式，线程分离模式
        pthread_detach(_tids[i]);
    }
    return;
}

thread_queue<task_msg> * thread_pool::get_thread()
{
    // 轮询返回
    if(_index == _thread_cnt)
        _index = 0;
    return _queues[_index++];
}

// 发送 NEW_TASK 类型 task的任务接口
void thread_pool::send_task(task_func func, void * args)
{
    // 给当前 thread_pool 中每一个 thread中的thread_queue发送当前task
    task_msg task;
    // 组装任务
    task.type = task_msg::NEW_TASK;
    task.task_cb = func;
    task.args = args;

    for(int i=0;i< _thread_cnt; i++)
    {
        // 取出 第i个线程的 消息队列
        thread_queue<task_msg> * queue = _queues[i];

        // 给queue 发送 一个task任务
        queue->send(task);
    }
}