# 目标
基于刘丹兵老师的《Lars系统》的一个即插即用的reactor服务器框架，之后结合C++语言版本的新特性随时补充，并添加新的高开放性的功能。

# 用途
主要是用于处理高并发和异步IO的解决方案，涉及内存管理、并发管理和消息业务管理等方面。

- 内存管理：实现自关联的io_buffer，支持链表方式组织。引入全局共享的内存池来进行管理，实现内存的高效分配和释放。
- 并发管理：封装原生epoll的实体，采用reactor框架的多reactor多线程的方案，实现多路IO机制，每个线程绑定一个该实体，提高效率。服务端使用线程池来进行连接管理，提高服务器性能。
- 消息业务管理：每个线程绑定队列来管理消息任务，实现消息高效处理。通过消息路由机制来关联消息和相关业务函数。


# 更新
- 结合C++11新特性的智能指针内容，提高安全性。

