#include "redismap.h"
#include <assert.h>
namespace ed{ namespace connectors{

RedisMap::RedisMap(const std::string& redis_connection, const std::string& region_name):
        redis(redis_connection, "", ""), region_name(region_name){

    init_logger();
    this->logger = log4cplus::Logger::getInstance("log");
    try{
        this->redis.redis_connect_with_timeout();
    }catch(const navitia::exception& ne){
        throw navitia::exception(ne.what());
    }
}

void RedisMap::vector_set_diff(std::vector<std::string> vect, std::vector<key_pair> set_,
                               std::function<void(const std::string&, const std::string&)> f1,
                               std::function<void(const std::string&)> f2) {
    auto key_redis = vect.begin();
    auto key_map = set_.begin();
    redis.start_pipeline();
    while(key_redis != vect.end() && key_map != set_.end()) {
        if(*key_redis == key_map->first) {
            ++key_redis; ++key_map;
        } else if(*key_redis > key_map->first) {
            f1(key_map->first, key_map->second);
            ++key_map;
        } else {
            f2(*key_redis);
            ++key_redis;
        }
    }
    for(;key_map!=set_.end(); ++key_map) {
        f1(key_map->first, key_map->second);
    }
    for(;key_redis != vect.end(); ++key_redis) {
        f2(*key_redis);
    }
    redis.end_pipeline();
}

void RedisMap::insert() {

    const auto separator = redis.get_separator();

    auto select_keys = [](std::pair<const key_pair, std::string>& key_value)-> key_pair{
        return key_value.first;
    };
    auto add_prefix_regions = [&](const key_pair& key)->key_pair{
        const auto first =  "places." + key.first + ".regions";
        return std::make_pair(first, key.second);
    };
    auto add_prefix_identifiers = [&](const key_pair& key)->key_pair{
        const auto first = "places." + key.first + ".identifiers";
        return std::make_pair(first, key.second);
    };

    //We keep the first part of keys
    std::vector<key_pair> keys;
    std::transform(this->redis_map.begin(), this->redis_map.end(),
            std::inserter(keys, keys.begin()), select_keys);

    //We insert for each object his region
    {
    std::vector<key_pair> keys_region;
    std::transform(keys.begin(), keys.end(),
                   std::inserter(keys_region, keys_region.begin()), add_prefix_regions);
    std::sort(keys_region.begin(), keys_region.end());
    auto keys_in_redis = redis.keys("places.*" + separator + ".regions");
    //We delete the keys that are in redis and not to insert
    vector_set_diff(keys_in_redis, keys_region, [](const std::string&, const std::string&){},
            [&](const std::string& str){
        redis.srem(str, this->region_name);
        if(redis.scard(str) == 0) {
            redis.del(str);
        }
    });

    //We insert the keys that are not in redis yet
    vector_set_diff(keys_in_redis, keys_region, [&](const std::string& str, const std::string&){
        redis.sadd(str, this->region_name);
    }, [](const std::string&){});
    }



    //We insert for each object his identifiers
    {
    std::vector<key_pair> keys_id;
    std::transform(keys.begin(), keys.end(), std::inserter(keys_id, keys_id.begin()),
            add_prefix_identifiers);
    auto keys_in_redis = redis.keys("places.*" + separator + ".identifiers");
    //We delete the keys that are in redis and not to insert
    vector_set_diff(keys_in_redis, keys_id, [](const std::string&, const std::string&){},
            [&](const std::string str){
        redis.hdel(str, this->region_name);
        if(redis.hlen(str) == 0) {
            redis.del(str);
        }
    });
    //We insert the keys that are not in redis yet
    vector_set_diff(keys_in_redis, keys_id, [&](const std::string key, const std::string type) {
        const auto str_key = key.substr(7, key.size() - 19);
        redis.hset(key, type, redis_map[std::make_pair(str_key, type)]);
    }, [](const std::string&){});
    }
}


void RedisMap::add_key_value(const std::string& key, const std::string &field_type,
    const std::string& value) {
    assert(value != "");
    auto key_type = std::make_pair(key, field_type);
    auto it_find = this->redis_map.find(key_type);
    if(it_find == this->redis_map.end()) {
        this->redis_map.insert(std::make_pair(key_type, value));
    } else {
        LOG4CPLUS_ERROR(this->logger, "For key: " + key + " the field " + field_type + " has already been add");
    }
}
}}
