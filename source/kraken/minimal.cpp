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
