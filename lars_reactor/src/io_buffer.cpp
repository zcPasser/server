#include "io_buf.h"

// #include <stdio.h>
#include <cassert>
#include <string.h>

io_buf::io_buf(int size)
{
    capacity = size;
    length = 0;
    head = 0;
    next = nullptr;

    data = new char[size];
    // 若 data == nullptr，程序直接退出
    assert(data);
}

//清空数据
void io_buf::clear()
{
    length = head = 0;
}

// 处理长度为len的数据，移动head
// len: 本次处理的数据长度
void io_buf::pop(int len)
{
    length -= len;
    head += len;
}
    
// 将head重置，将已经处理的数据清空，将未处理的数据 前移至首地址，length减小。
void io_buf::adjust()
{
    if(head != 0)
    {
        if(length != 0)
        {
            memmove(data, data + head, length);
        }
        head = 0;
    }
}

// 将其他io_buf对象拷贝到自身
void io_buf::copy(const io_buf*other)
{
    memcpy(data, other->data + other -> head, other -> length);
    head = 0;
    length = other -> length;
}