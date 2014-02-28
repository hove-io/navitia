#pragma once
#include <utils/logger.h>
#include <boost/filesystem.hpp>
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
    std::string path;///< Path files
    log4cplus::Logger logger;
    std::vector<std::string> files;
    ed::connectors::ConvCoord conv_coord;

    size_t ways_fusionned;

    ed::types::Node* add_node(const navitia::type::GeographicalCoord& coord, const std::string& uri);
    void fill_admins();
    void fill_nodes();
    void fill_ways_edges();
    void fill_house_numbers();
    void fill_poi_types();
    void fill_pois();
    bool starts_with(std::string filename, const std::string& prefex);
    void fusion_ways();
    void fusion_ways_by_graph(std::vector<types::Edge*>& ways);
    void fusion_ways_list(const std::multimap<size_t, ed::types::Edge*>& component_edges);
    ed::types::Way* get_way(const std::string& old_uri) const;

public:
    ed::Georef data;

    GeopalParser(const std::string& path, const ed::connectors::ConvCoord& conv_coord);
    void fill();

};
}//namespace connectors
}//namespace ed

