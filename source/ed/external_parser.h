#pragma once
#include <string>
#include "data.h"
#include <utils/logger.h>

//namespace navitia{ namespace georef{
namespace ed{ namespace connectors{
class ExternalParser
{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;

public:
    ExternalParser(const std::string & path);
    ExternalParser(): path(){}

    void fill_aliases(Data & data);
    void fill_synonyms(Data & data);
    void fill_alias_synonyme(Data & data);
};
}}
