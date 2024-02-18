#pragma once

#include <string>
#include <vector>
#include <map>

// map: 存放配置信息
// key: type:string 存放标题section
// value: type:map 存放该标题下所有key-value键值对
typedef std::map<std::string, std::map<std::string, std::string> *> STR_MAP;

typedef STR_MAP::iterator STR_MAP_ITER;

// 模式: 单例模式
class config_file
{
public:
    ~config_file();

    // 获取 字符串类型 配置信息
    std::string get_string(const std::string& section, const std::string& key, const std::string& default_value="");

    // 字符串集合配置信息
    std::vector<std::string> get_string_list(const std::string& section, const std::string& key);

    // 获取整型类型配置信息
    unsigned get_number(const std::string& section, const std::string& key, unsigned default_value=0);

    // 获取布尔类型配置信息
    unsigned get_bool(const std::string& section, const std::string& key, bool default_value=false);

    // 获取浮点类型配置信息
    unsigned get_float(const std::string& section, const std::string& key, const float& default_value);

    // 设置配置文件所在路径
    static bool set_path(const std::string& path);

    // 获取单例
    static config_file* instance();

private:
    // 私有构造
    config_file() {}

    // 字符串配置文件解析基础方法
    bool is_section(std::string line, std::string& section);
    unsigned parse_number(const std::string& s);
    std::string trim_left(const std::string& s);
    std::string trim_right(const std::string& s);
    std::string trim(const std::string& s);
    bool load(const std::string& path);

    // 唯一 读取配置文件实例
    static config_file* config;

    STR_MAP _map;
};

