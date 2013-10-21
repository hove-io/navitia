#pragma once
#include <string>
#include "type/data.h"
#include <utils/logger.h>

namespace ed{ namespace connectors{

class PoiParser
{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;

public:
    PoiParser(const std::string & path);
    PoiParser(): path(){}

    void fill_poi_type(navitia::georef::GeoRef & georef_to_fill);
    void fill_poi(navitia::georef::GeoRef & georef_to_fill);
    void fill(navitia::georef::GeoRef & georef_to_fill);
    void fill_aliases(navitia::georef::GeoRef & georef_to_fill);
    void fill_synonyms(navitia::georef::GeoRef & georef_to_fill);
    void fill_alias_synonyme(navitia::georef::GeoRef & georef_to_fill);
};
}}


