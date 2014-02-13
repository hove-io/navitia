#pragma once
#include <utils/logger.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "utils/csv.h"
#include "utils/functions.h"
#include "ed/data.h"
#include "ed/connectors/conv_coord.h"


namespace ed{ namespace connectors{

struct GeopalParserException: public navitia::exception{
    GeopalParserException(const std::string& message): navitia::exception(message){};
};

class GeopalParser{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;
    std::vector<std::string> files;
    ed::connectors::ConvCoord conv_coord;

    ed::Georef data;
    ed::types::Node* add_node(ed::Georef& data, const navitia::type::GeographicalCoord& coord, const std::string& uri);
    void fill_admins(ed::Georef& data);
    void fill_nodes(ed::Georef& data);
    void fill_ways_edges(ed::Georef& data);
    void fill_House_numbers(ed::Georef& data);
    bool starts_with(std::string filename, const std::string& prefex);

public:
    GeopalParser(const std::string& path, const ed::connectors::ConvCoord& conv_coord);

    void fill(ed::Georef& data);

};
}
}

