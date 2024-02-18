#pragma once

#include "io_buf.h"
#include "buf_pool.h"

class reactor_buf
{
public:
    reactor_buf();
    ~reactor_buf();

    // 当前buf 还有多少 有效数据
    int length();
    // 已经 处理了 多少数据
    void pop(int len);
    // 
    void clear();
protected:
    io_buf* _buf;
};

// input_buf
class input_buf : public reactor_buf
{
public:
    // 从fd读取数据到reactor_buf
    int read_data(int fd);

    // 获取当前数据
    const char* data();

    // 重置缓冲区
    void adjust();
};

// output_buf
class output_buf : public reactor_buf
{
public:
    // 将一段 数据 写到 自身的 _buf
    int send_data(const char* data, int data_len);

    // 将从reactor_buf继承的_buf 数据 写入 fd
    // 取代I/O层write方法
    int write2fd(int fd);

};