#pragma once
#include<string>
#include "ed/data.h"
#include"type/type.h"
#include "utils/redis.h"
#include "utils/logger.h"

namespace ed{
namespace connectors{

    class ExtCode2Uri{
    public:
        Redis redis;
        log4cplus::Logger logger;

        ExtCode2Uri(const std::string& redis_connection);
        void to_redis(const ed::Data & data);

    };
}

template<typename T>
void add_redis(const std::vector<T*>& vec, connectors::ExtCode2Uri& extcode2uri){
    extcode2uri.redis.set_prefix(navitia::type::static_data::get()->captionByType(T::type));
    for(auto* element : vec){
        try{
            extcode2uri.redis.set(element->external_code, element->uri);
        }catch(const navitia::exception& ne){
            LOG4CPLUS_WARN(extcode2uri.logger, "add_redis : " + std::string(ne.what()));
        }
    }
}
}
