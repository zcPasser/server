#pragma once
#include "event_loop.h"
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <sys/eventfd.h>

template <typename T>
class thread_queue
{
public:
    thread_queue()
    {
        _loop = nullptr;
        pthread_mutex_init(& _queue_mutex, nullptr);
        // 创建 fd，用来被epoll监听,没有和 磁盘相关联 ，也没有和socket(网卡)相关联
        _evfd = eventfd(0, EFD_NONBLOCK);
        if(_evfd == -1)
        {
            fprintf(stderr, "evenfd error\n");
            exit(1);
        }
    }

    ~thread_queue()
    {
        pthread_mutex_destroy(&_queue_mutex);
        close(_evfd);
    }

    // 向 队列中 添加 任务（main_thread 调用 或者 其他业务功能线程来调用）
    void send(const T& task)
    {
        // 将 task 加入到 queue中，激活 _evfd
        pthread_mutex_lock(&_queue_mutex);
        _queue.push(task);

        // 激活 _evfd 可读事件，向 _evfd 写数据
        // 触发消息事件的占位传输内容
        unsigned long long idle_num = 1;
        int ret = write(_evfd, &idle_num, sizeof(unsigned long long));
        if(ret == -1)
        {
            fprintf(stderr, "write _evfd error\n");
        }

        pthread_mutex_unlock(&_queue_mutex);
    }

    // 从队列中 取数据，将整个 queue 返回给 上层,que 传出型参数
    void recv(std::queue<T>& que)
    {
        // 激活 _evfd 可读事件，向 _evfd 写数据
        // 触发消息事件的占位传输内容
        unsigned long long idle_num = 1;

        // 取出 queue 数据
        pthread_mutex_lock(&_queue_mutex);
        
        int ret = read(_evfd, &idle_num, sizeof(unsigned long long));
        if(ret == -1)
        {
            fprintf(stderr, "read _evfd error\n");
        }
        // 将 evfd 绑定的 事件读写 缓存清空，将 占位激活符合
        // 交换 两个容器的 指针,要先保证 que是空，交换后，_queue才为空。
        std::swap(que, _queue);
        // std::copy(que, _queue) 效率低

        pthread_mutex_unlock(&_queue_mutex);
    }

    // 监听当前 thread_queue的 event_loop
    void set_loop(event_loop* loop)
    {
        this->_loop = loop;
    }

    void set_callback(io_callback cb, void * args=nullptr)
    {
        if(_loop != nullptr)
        {
            _loop->add_io_event(_evfd, cb, EPOLLIN, args);
        }
    }
private:
    //线程监听的文件描述符，连接
    int _evfd;
    // 当前被哪个loop监听
    event_loop * _loop;
    // 队列
    std::queue<T> _queue;
    // 保护 queue的 互斥锁
    pthread_mutex_t _queue_mutex;
};