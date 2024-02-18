
#include "config_file.h"

#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <strings.h>

config_file* config_file::config = nullptr;

config_file::~config_file()
{
    for(STR_MAP_ITER it = _map.begin(); it != _map.end(); ++it)
    {
        delete it->second;
    }
}

// 获取 字符串类型 配置信息
std::string config_file::get_string(const std::string& section, const std::string& key, const std::string& default_value)
{
    STR_MAP_ITER it = _map.find(section);

    if(it != _map.end())
    {
        std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
        if(it1 != it->second->end())
            return it1->second;
    }
    return default_value;
}

// 字符串集合配置信息
std::vector<std::string> config_file::get_string_list(const std::string& section, const std::string& key)
{
    std::vector<std::string> v;
    // 获取配置文件中对应键的字符串值
    std::string str = get_string(section, key, "");
    // 定义分隔符，这里使用逗号、空格和制表符
    std::string sep = ", \t";
    std::string substr;
    std::string::size_type start = 0;
    std::string::size_type index;

    while((index = str.find_first_of(sep, start)) != std::string::npos)
    {
        
        substr = str.substr(start, index - start);
        v.push_back(substr);

        // 更新起始位置为下一个非分隔符的位置
        start = str.find_first_not_of(sep, index);

        // 如果没有找到下一个非分隔符，则返回当前向量
        if(start == std::string::npos)
            return v;
    }
    // 处理最后一个子串（或者整个字符串就是一个子串）
    substr = str.substr(start);
    v.push_back(substr);

    return v;
}

// 获取整型类型配置信息
unsigned config_file::get_number(const std::string& section, const std::string& key, unsigned default_value)
{
    STR_MAP_ITER it = _map.find(section);

    if(it != _map.end())
    {
        std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
        if(it1 != it->second->end())
            return parse_number(it1->second);
    }
    return default_value;
}

// 获取布尔类型配置信息
unsigned config_file::get_bool(const std::string& section, const std::string& key, bool default_value)
{
    STR_MAP_ITER it = _map.find(section);
    if(it != _map.end())
    {
        std::map<std::string, std::string>::const_iterator it1 = it->second->find(key);
        if(it1 != it->second->end())
        {
            if(strcasecmp(it1->second.c_str(), "true") == 0)
                return true;
        }
    }
    return default_value;
}

// 获取浮点类型配置信息
unsigned config_file::get_float(const std::string& section, const std::string& key, const float& default_value)
{
    std::ostringstream convert1;
    convert1 << default_value;
    // 将 浮点数 转换成 字符串，然后 按照 字符串 业务处理
    std::string default_value_str = convert1.str();
    std::string text = get_string(section, key, default_value_str);
    std::istringstream convert2(text);

    float fresult;
    // 如果Result放不下text对应的数字，执行将返回0；
    if(!(convert2 >> fresult))
        fresult = 0;

    return fresult;
}

// 解析配置文件 并 设置配置文件所在路径
bool config_file::set_path(const std::string& path)
{
    assert(config == nullptr);
    // 
    config = new config_file();
    return config->load(path);
}

// 获取单例
config_file* config_file::instance()
{
    assert(config != nullptr);
    return config;
}

// 字符串配置文件解析基础方法
bool config_file::is_section(std::string line, std::string& section)
{
    section = trim(line);

    if(section.empty() || section.length() <= 2)
        return false;

    if(section.at(0) != '[' || section.at(section.length() - 1) != ']')
        return false;

    section = section.substr(1, section.length() - 2);
    section = trim(section);

    return true;
}

unsigned config_file::parse_number(const std::string& s)
{
    std::istringstream iss(s);
    long long v = 0;
    iss >> v;
    return v;
}

std::string config_file::trim_left(const std::string& s)
{
    size_t first_not_empty = 0;

    std::string::const_iterator beg = s.begin();

    while(beg != s.end())
    {
        if(!isspace(*beg))
            break;
        first_not_empty++;
        beg++;
    }
    return s.substr(first_not_empty);
}

std::string config_file::trim_right(const std::string& s)
{
    size_t last_not_empty = s.length();
    std::string::const_iterator end = s.end();
    while (end != s.begin())
    {
        end--;
        if (!isspace(*end))
        {
            break;   
        }
        last_not_empty--;
    }
    return s.substr(0, last_not_empty);
}

std::string config_file::trim(const std::string& s)
{
    return trim_left(trim_right(s));
}

bool config_file::load(const std::string& path)
{
    // 打开配置文件
    std::ifstream ifs(path.c_str());
    if(!ifs.good())
        return false;
    std::string line;
    std::map<std::string, std::string> *m=nullptr;

    // 逐行读取配置文件内容
    while(!ifs.eof() && ifs.good())
    {
        // 读取一行
        getline(ifs, line);
        std::string section;

        // 判断是否为节（section）行
        if(is_section(line, section))
        {
            // 如果是节行，检查是否已存在该节，如果不存在则创建
            STR_MAP_ITER it = _map.find(section);
            if(it == _map.end())
            {
                m = new std::map<std::string, std::string>();
                _map.insert(STR_MAP::value_type(section, m));
            }
            else
            {
                m = it -> second;
            }
            continue;
        }

        // 如果不是节行，继续解析键值对
        size_t equ_ops = line.find('=');
        if(equ_ops == std::string::npos)
            continue;

        // 分割键值对
        std::string key = line.substr(0, equ_ops);
        std::string value = line.substr(equ_ops + 1);
        key = trim(key);
        value = trim(value);

        // 忽略空键
        if(key.empty())
            continue;
        if(key[0] == '#' || key[0] == ';')
            continue;
        
        // 查找并更新或插入键值对
        std::map<std::string, std::string>::iterator it1 = m->find(key);
        if(it1 != m->end())
            it1->second = value;
        else
            m->insert(std::map<std::string, std::string>::value_type(key, value));
    }

    // 关闭文件流
    ifs.close();

    return true;
}
