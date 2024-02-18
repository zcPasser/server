#pragma once

#include "mysql.h"

#include <pthread.h>
#include <ext/hash_map>
#include <ext/hash_set>

using __gnu_cxx::hash_map;
using __gnu_cxx::hash_set;

// ========= auxiliary =========
//定义用来保存host的IP/host的port的的集合 数据类型
typedef hash_set<uint64_t> host_set;
typedef hash_set<uint64_t>::iterator host_set_it;

//定义用来保存modID/cmdID与host的IP/host的port的对应的关系 数据类型  
typedef hash_map< uint64_t, host_set > route_map;
typedef hash_map< uint64_t, host_set >::iterator route_map_it;

//backendThread main
void *check_route_changes(void *args);
// ========= end =========

// modid/cmdid ---> ip/port
class Route
{
public:
    // 创建 单例
    static void init()
    {
        _instance = new Route();
    }

    // 获取 单例
    static Route* instance()
    {
        pthread_once(&_once, init);
        return _instance;
    }

    // 连接数据库方法
    void connect_db();

    // 构建 map route数据方法
    // 将RouteData表中数据加载到内存map
    void build_maps();

    // 通过 modid/cmdid 获取 当前模块挂载的全部host集合
    host_set get_hosts(int modid, int cmdid);

    //获取当前Route版本号
    int load_version();

    //加载RouteData到_temp_pointer
    int load_route_data();

    //将temp_pointer的数据更新到data_pointer
    void swap();

    //加载RouteChange得到修改的modid/cmdid
    //将结果放在vector中
    void load_changes(std::vector<uint64_t>& change_list);

    //将RouteChange
    //删除RouteChange的全部修改记录数据,remove_all为全部删除
    //否则默认删除当前版本之前的全部修改
    void remove_changes(bool remove_all = false);
private:
    // 全部构造函数私有化
    Route();
    Route(const Route&);
    const Route& operator=(const Route&);

    // 单例锁
    static pthread_once_t _once;

    // 指向 单例对象的 指针
    static Route * _instance;

    // mysql数据库句柄
    MYSQL _db_conn;

    // 表句柄
    // 指向 RouteDataMap_A 当前关系map
    route_map * _data_pointer;
    // 指向 RouteDataMap_B 临时关系map
    route_map * _temp_pointer;
    // map 读写锁
    pthread_rwlock_t _map_lock;

    // 当前版本号
    long _version;

    char _sql[1000];
};