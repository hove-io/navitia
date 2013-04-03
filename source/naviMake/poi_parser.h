#pragma once
#include <string>
#include "type/data.h"
#include <utils/logger.h>



namespace navitia{ namespace georef{
class PoiParser
{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;

public:
    PoiParser(const std::string & path);
    PoiParser(): path(){}

    void fill_poi_type(GeoRef & georef_to_fill);
    void fill_poi(GeoRef & georef_to_fill);
    void fill(GeoRef & georef_to_fill);
    void fill_aliases(GeoRef & georef_to_fill);
    void fill_synonyms(GeoRef & georef_to_fill);
    void fill_alias_synonyme(GeoRef & georef_to_fill);
};
}}


