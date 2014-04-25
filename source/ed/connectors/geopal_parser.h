/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

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

    ed::types::Node* add_node(const navitia::type::GeographicalCoord& coord, const std::string& uri);
    void fill_admins();
    void fill_nodes();
    void fill_ways_edges();
    void fill_house_numbers();
    bool starts_with(std::string filename, const std::string& prefex);
    void fusion_ways();
    void fusion_ways_by_graph(const std::vector<types::Edge*>& edges);
    void fusion_ways_list(const std::vector<types::Edge*>& edges);

public:
    ed::Georef data;

    GeopalParser(const std::string& path, const ed::connectors::ConvCoord& conv_coord);
    void fill();

};
}//namespace connectors
}//namespace ed

