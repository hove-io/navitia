#pragma once
#include <string>
#include <utils/logger.h>
#include <utils/exception.h>

namespace ed{ namespace connectors{
struct SynonymParserException: public navitia::exception{
    SynonymParserException(const std::string& message): navitia::exception(message){};
};

class SynonymParser{
private:
    log4cplus::Logger logger;
    std::string filename;
    void fill_synonyms();

public:
    std::map<std::string, std::string> synonym_map;

    SynonymParser(const std::string& filename);

    void fill();
};
}}
