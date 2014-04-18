#pragma once
#include <string>
#include "ed/data.h"
#include <utils/logger.h>

//namespace navitia{ namespace georef{
namespace ed{ namespace connectors{
class ExternalParser
{
private:
    log4cplus::Logger logger;

public:
    ExternalParser();

    void fill_synonyms(const std::string &file, Data & data);
};
}}
