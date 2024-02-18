#pragma once

/*
    定义一个buffer一块内存的数据结构体
 */

class io_buf
{
public:
    // 构造函数
    io_buf(int size);

    //清空数据
    void clear();

    // 处理长度为len的数据，移动head
    // len: 本次处理的数据长度
    void pop(int len);
    
    // 将head重置，将已经处理的数据清空，将未处理的数据 前移至首地址，length减小。
    void adjust();

    // 将其他io_buf对象拷贝到自身
    void copy(const io_buf*other);

    // 当前buf的总容量
    int capacity;
    // 当前buf的有效数据长度
    int length;
    // 当前buf中未处理数据的头部索引
    int head;
    // 当前buf 的 内存首地址
    char* data;

    io_buf* next;
};