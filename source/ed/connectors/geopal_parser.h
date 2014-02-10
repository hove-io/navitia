#pragma once
#include <utils/logger.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "utils/csv.h"
#include "utils/functions.h"
#include "ed/data.h"


namespace ed{ namespace connectors{

class GeopalParser{
private:
    std::string path;///< Chemin vers les fichiers
    log4cplus::Logger logger;
    std::vector<std::string> files;

    ed::Georef data;
    ed::types::Node* add_node(ed::Georef& data, const navitia::type::GeographicalCoord& coord, const std::string& uri);
    void fill_admins(ed::Georef& data);
    void fill_nodes(ed::Georef& data);
    void fill_ways_edges(ed::Georef& data);
    void fill_House_numbers(ed::Georef& data);

public:
    GeopalParser(const std::string& path);

    void fill(ed::Georef& data);

};
}
}

