#pragma once

#include <mutex>
#include <pthread.h>
#include <ext/hash_map>
#include "io_buf.h"

typedef __gnu_cxx::hash_map<int, io_buf*> pool_t;

// 定义 内存刻度
enum MEM_CAP
{
    m4K = 4096,
    m16K = 16384,
    m64K = 65536,
    m256K = 262144,
    m1M = 1048576,
    m4M = 4194304,
    m8M = 8388608,
};

// 定义 总内存池的大小限制 5G，下面的单位是KB
#define MEM_LIMIT (5U * 1024 * 1024)

class buf_pool
{
public:
    // 初始化 单例对象
    static void init()
    {
        _instance = new buf_pool();
    }
    // 获取 instance的静态方法
    static buf_pool* get_instace()
    {   
        // 保证 init方法 在进程的生命周期中 只执行一次。
        // pthread_once(&_once, init);

        // 这里使用了两个 if 判断语句的技术称为双检锁；
        // 好处是，只有判断指针为空的时候才加锁，
        // 避免每次调用 GetInstance的方法都加锁，锁的开销毕竟还是有点大的。
        if(_instance == nullptr)
        {
            std::unique_lock<std::mutex> lock(_once); // 加锁
            if (_instance == nullptr)
            {
                volatile auto temp = new (std::nothrow) buf_pool();
                _instance = temp;
            }
        }
        return _instance;
    }

    static void delete_instance()
    {
        std::unique_lock<std::mutex> lock(_once); // 加锁
        if (_instance)
        {
            delete _instance;
            _instance = nullptr;
        }
    }

    // 向 内存池 申请 内存
    io_buf * alloc_buf(int N);
    io_buf * alloc_buf();

    // 重置
    void revert(io_buf* buffer);

    // 开辟 内存池中一种刻度内存
    void make_io_buf_list(int capacity, int nums);
private:
    // ============== 创建单例模式 ==============
    // 构造函数 私有化
    buf_pool();
    buf_pool(const buf_pool&);
    const buf_pool& operator=(const buf_pool&);
    // 单例对象
    static buf_pool* _instance;

    // 保证 创建单例的一个方法全局只执行一次
    // static pthread_once_t _once;
    
    // 单例模式实例创建 加锁
    // static pthread_mutex_t _once;
    static std::mutex _once;
    // 加锁，保护 pool map增删改查
    static pthread_mutex_t _mutex;

    // ============== 业务属性 ==============
    // 所有io_buf的map句柄
    pool_t _pool;
    // 当前内存池总大小
    uint64_t _total_mem;
};