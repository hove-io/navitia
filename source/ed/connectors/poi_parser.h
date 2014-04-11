#pragma once
#include "ed/data.h"
#include <utils/logger.h>
#include "ed/connectors/conv_coord.h"

namespace ed{ namespace connectors{

struct PoiParserException: public navitia::exception{
    PoiParserException(const std::string& message): navitia::exception(message){};
};


class PoiParser
{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;
    ed::connectors::ConvCoord conv_coord;

    void fill_poi_type();
    void fill_poi();
    void fill_poi_properties();
public:
    ed::PoiPoiType data;

    PoiParser(const std::string & path, const ed::connectors::ConvCoord& conv_coord);
    void fill();
};
}}


