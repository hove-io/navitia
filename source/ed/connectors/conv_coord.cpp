#include "conv_coord.h"

namespace ed{ namespace connectors{

Projection::Projection(const std::string& name, const std::string& num_epsg, bool is_degree): name(name), is_degree(is_degree){
    this->definition = "+init=epsg:" + num_epsg;
}

navitia::type::GeographicalCoord ConvCoord::convert_to(navitia::type::GeographicalCoord coord) const{

    projPJ pj_src = pj_init_plus(this->origin.definition.c_str());
    projPJ pj_dest = pj_init_plus(this->destination.definition.c_str());

    if(this->origin.is_degree){
        coord.set_lon(coord.lon() * DEG_TO_RAD);
        coord.set_lat(coord.lat() * DEG_TO_RAD);
    }
    double x = coord.lon();
    double y = coord.lat();
    pj_transform(pj_src, pj_dest, 1, 1,&x, &y, NULL);
    coord.set_lon(x);
    coord.set_lat(y);
    if(this->destination.is_degree){
        coord.set_lon(coord.lon() * RAD_TO_DEG);
        coord.set_lat(coord.lat() * RAD_TO_DEG);
    }

    pj_free(pj_dest);
    pj_free(pj_src);
    return coord;
}

}//namespace connectors
}//namespace ed
