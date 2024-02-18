#include <pthread.h>
#include <cassert>

#include "buf_pool.h"

// 单例对象
buf_pool* buf_pool::_instance = nullptr;

// 保证 创建单例的一个方法全局只执行一次
// static pthread_once_t _once;
    
// 加锁
std::mutex buf_pool::_once;
// pthread_mutex_init(&_once, NULL);

// 加锁，保护 pool map增删改查
// 初始化
pthread_mutex_t buf_pool::_mutex = PTHREAD_MUTEX_INITIALIZER;

// 开辟指定刻度 内存链表
void buf_pool::make_io_buf_list(int capacity, int nums)
{
    // _total_mem = 0;
    io_buf* prev;
    // 开辟 capacity 内存池
    _pool[capacity] = new io_buf(capacity);
    if (_pool[capacity] == nullptr)
    {
        fprintf(stderr, "new io_buf %d error", capacity);
        exit(1);
    }
    prev = _pool[capacity];

    // capacity 类型的buf 预先开辟nums个，约capacity * nums供开发者使用
    for(int i=1;i < nums; i++)
    {
        prev->next = new io_buf(capacity);
        if(prev->next == nullptr)
        {
            fprintf(stderr, "new io_buf %d error", capacity);
            exit(1);   
        }
        prev = prev->next;
    }
    _total_mem += capacity / 1024 * nums;
    
    // return _pool[capacity];
}

// 构造函数 私有化
buf_pool::buf_pool()
{
    _total_mem = 0;
    // 开辟 4K 内存池
    make_io_buf_list(m4K, 5000);
    make_io_buf_list(m16K, 1000);
    make_io_buf_list(m64K, 500);
    make_io_buf_list(m256K, 200);
    make_io_buf_list(m1M, 50);
    make_io_buf_list(m4M, 20);
    make_io_buf_list(m8M, 10);
}

// 向 内存池 申请 内存
io_buf * buf_pool::alloc_buf(int N)
{
    // 1-判断 和N 最相近的 内存刻度
    int idx;
    if(N <= m4K)
        idx=m4K;
    else if(N<=m16K)
        idx=m16K;
    else if(N<=m64K)
        idx=m64K;
    else if(N<=m256K)
        idx=m256K;
    else if(N<=m1M)
        idx=m1M;
    else if(N<=m4M)
        idx=m4M;
    else if(N<=m8M)
        idx=m8M;
    else
        return nullptr;
    // 2-如果该 内存刻度的 内存不够，则内存池额外开辟。
    pthread_mutex_lock(&_mutex);
    if(_pool[idx] == nullptr)
    {
        // 判断 加上本次内存 是否 超过 内存限制
        if(_total_mem + N / 1024 >= MEM_LIMIT)
        {
            fprintf(stderr, "already use too many memory!");
            exit(1);
        }
        io_buf* new_buf = new io_buf(idx);
        if(new_buf == nullptr)
        {
            fprintf(stderr, "new io_buff %d error!\n", idx);
            exit(1);
        }  
        _total_mem += idx / 1024;   
        pthread_mutex_unlock(&_mutex);
        return new_buf;   
    }
    // 3-内存刻度的内存足够，则返回。
    io_buf* target = _pool[idx];
    _pool[idx] = target->next;
    target->next = nullptr;
    pthread_mutex_unlock(&_mutex);
    
    return target;
}

// 默认
io_buf * buf_pool::alloc_buf()
{
    return alloc_buf(m4K);
}

// 重置
void buf_pool::revert(io_buf* buffer)
{
    // 判断 buffer的大小 属于 何种 内存刻度
    int idx = buffer->capacity;
    buffer->length = 0;
    buffer->head = 0;

    // 对共享数据 内存池进行操作，加锁
    pthread_mutex_lock(&_mutex);
    // 断言 一定能在 内存池中 找到 该 内存刻度
    assert(_pool.find(idx) != _pool.end());
#if 0
    if(_pool.find(idx) == _pool.end())
    {
        exit(1);
    }
#endif
    buffer->next = _pool[idx];
    _pool[idx] = buffer;
    pthread_mutex_unlock(&_mutex);
}
