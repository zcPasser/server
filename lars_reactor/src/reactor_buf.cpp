#include "reactor_buf.h"

#include <cassert>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
// #include <assert.h>

reactor_buf::reactor_buf()
{
    _buf = nullptr;
}
reactor_buf::~reactor_buf()
{
    this->clear();
}

// 当前buf 还有多少 有效数据
int reactor_buf::length()
{
    if(_buf == nullptr)
        return 0;
    else
        return _buf -> length;
}

// 已经 处理了 多少数据
void reactor_buf::pop(int len)
{
    assert(_buf != nullptr && len <= _buf->length);

    _buf->pop(len);
    if(_buf->length == 0)
    {
        this->clear();
    }
}

// 
void reactor_buf::clear()
{
    if(_buf != nullptr)
    {
        buf_pool::get_instace()->revert(_buf);
        _buf = nullptr;
    }
}

// ======= input_buf =======
// 从fd读取数据到reactor_buf
int input_buf::read_data(int fd)
{
    // 硬件中有多少数据是可以读的。
    int need_read;    

    // 一次性读出所有的数据
    // 需要 给fd设置一个属性 FIONREAD
    // 传出一个参数，目前缓冲中 一共有 多少数据是可读的。
    if(ioctl(fd,FIONREAD, &need_read) == -1)
    {
        fprintf(stderr, "ioctl FIONREAD\n");
        return -1;
    }
    if(_buf == nullptr)
    {
        // 如果当前的input_buf中的_buf是空的，需要从buf_pool拿一个新的
        _buf = buf_pool::get_instace()->alloc_buf(need_read);
        if(_buf == nullptr)
        {
            fprintf(stderr, "no buf for alloc!\n");
            return -1;
        }
    }
    else
    {
        // 如果当前_buf可用，判断_buf是否 够存。
        assert(_buf->head==0);
        if(_buf ->capacity - _buf->length < need_read)
        {
            // 不够存
            io_buf * new_buf = buf_pool::get_instace()->alloc_buf(need_read + _buf->length);
            if(new_buf==nullptr)
            {
                fprintf(stderr, "no buf for alloc!\n");
                return -1;
            }
            new_buf->copy(_buf);
            // 将之前的_buf放回内存池中
            buf_pool::get_instace()->revert(_buf);
            _buf = new_buf;
        }
    }

    // 读取数据
    int already_read=0;
    do 
    {
        // 读取的数据拼接到之前的数据之后
        if(need_read == 0)
        {
            // 可能是read阻塞读数据的模式，对方未写数据
            already_read = read(fd, _buf->data + _buf->length, m4K);
        }
        else
        {
            already_read = read(fd, _buf->data + _buf->length, need_read);
        }
    // systemCall引起的中断 继续读取
    }while(already_read == -1 && errno == EINTR);
    
    if(already_read > 0)
    {
        if(need_read != 0)
        {
            assert(already_read == need_read);
        }
        _buf -> length += already_read;
    }
    
    return already_read;
}

// 获取当前数据
const char* input_buf::data()
{
    return _buf !=nullptr ? _buf->data + _buf -> head : nullptr;
}

// 重置缓冲区
void input_buf::adjust()
{
    if(_buf !=nullptr)
    {
        _buf->adjust();
    }
}

// ======= output_buf =======
// 将一段 数据 写到 自身的 _buf
int output_buf::send_data(const char* data, int data_len)
{
    if(_buf == nullptr)
    {
        // 如果当前的output_buf中的_buf是空的，需要从buf_pool拿一个新的
        _buf = buf_pool::get_instace()->alloc_buf(data_len);
        if(_buf == nullptr)
        {
            fprintf(stderr, "no buf for alloc!\n");
            return -1;
        }
    }
    else
    {
        // 如果当前_buf可用，判断_buf是否 够存。
        assert(_buf->head==0);
        if(_buf ->capacity - _buf->length < data_len)
        {
            // 不够存
            io_buf * new_buf = buf_pool::get_instace()->alloc_buf(data_len + _buf->length);
            if(new_buf==nullptr)
            {
                fprintf(stderr, "no buf for alloc!\n");
                return -1;
            }
            new_buf->copy(_buf);
            // 将之前的_buf放回内存池中
            buf_pool::get_instace()->revert(_buf);
            _buf = new_buf;
        }
    }
    
    // 开始 写数据
    memcpy(_buf->data + _buf->length, data, data_len);

    _buf->length += data_len;
    return 0;
}
    
// 将从reactor_buf继承的_buf 数据 写入 fd
// 取代I/O层write方法
int output_buf::write2fd(int fd)
{
    assert(_buf != nullptr && _buf->head == 0);
    int already_write = 0;
    do
    {
        already_write = write(fd, _buf->data, _buf->length);
    }while(already_write == -1 && errno == EINTR);
    
    if(already_write > 0)
    {
        // 已经写成功
        _buf->pop(already_write);
        _buf->adjust();
    }
    // 若 fd非阻塞。already_write == -1 && errno == EAGAIN
    if(already_write == -1 && errno == EAGAIN)
    {
        // 不是错误，仅仅返回0，表示目前是不可以继续写的
        already_write = 0;
    }

    return already_write;
}
