#pragma once
#include<string>
#include "ed/data.h"
#include"type/type.h"
#include "utils/redis.h"
#include "utils/logger.h"
#include "utils/exception.h"
#include <boost/functional/hash.hpp>


namespace ed{
namespace connectors{
struct RedisMap {
    RedisMap(const std::string& connection_string, const std::string& region_name);
    void add_key_value(const std::string& key, const std::string &field_type,
            const std::string& value);
    void insert();

    private:
        using key_pair = std::pair<std::string, std::string>;
        Redis redis;
        log4cplus::Logger logger;
        std::unordered_map<key_pair, std::string, boost::hash<key_pair>> redis_map;
        std::string region_name;

        void get_keys_of_region();
        void vector_set_diff(std::vector<std::string> vect, std::vector<key_pair> set_,
                               std::function<void(const std::string&, const std::string&)> f1,
                               std::function<void(const std::string&)> f2);
};
}}
