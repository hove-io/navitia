#pragma once
#include <hiredis/hiredis.h>
#include<string>
#include "utils/logger.h"
#include "utils/exception.h"
#include "utils/functions.h"
#include "ed/data.h"
#include"type/type.h"

namespace ed{ namespace connectors{
class ExtCode2Uri
    {
private:
    std::string host;
    std::string password;
    int port;
    int db;
    std::string prefix;
    std::string separator;
    log4cplus::Logger logger;

    redisContext* connect;

public:
        ExtCode2Uri(const std::string& redis_connection);
        ~ExtCode2Uri();
        bool connected();
        bool set(const std::string& key, const std::string& value);
        bool set_lists(const ed::Data & data);
    };
}
}


