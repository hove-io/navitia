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

#include "type/data.h"
#include "type/type.pb.h"
#include "type/pb_converter.h"
#include "interface/pb_utils.h"

int main(int, char**) {


    navitia::type::GeographicalCoord g(0.4,0.4);
    pbnavitia::GeographicalCoord *pb_g = new pbnavitia::GeographicalCoord();
    pb_g->set_lat(g.x);
    pb_g->set_lon(g.y);
    pb2json(pb_g, 0);

    navitia::type::Data data;
    data.geo_ref.ways.push_back(navitia::georef::Way());
    data.pt_data.cities.push_back(navitia::type::City());
    data.pt_data.stop_points.push_back(navitia::type::StopPoint());
    navitia::georef::Path path;
    path.path_items.push_back(navitia::georef::PathItem());
    path.path_items.front().way_idx = 0;
    path.path_items.front().length = 100.2;
    path.length  = 823.1;

    pbnavitia::Section* sn = new pbnavitia::Section();
    navitia::fill_road_section(path, data, sn);
    std::cout << pb2json(sn, 0);



    delete pb_g;
    delete sn;
    return 0;
}
