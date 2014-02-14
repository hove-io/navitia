#include "conv_coord.h"

namespace ed{ namespace connectors{

Projection::Projection(const std::string& name, const std::string& num_epsg, bool is_degree){
    definition = "+init=epsg:" + num_epsg;
    proj_pj = pj_init_plus(definition.c_str());
    this->name = name;
    this->is_degree = is_degree;

}

Projection::Projection(const Projection& other) {
    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    //we allocate a new proj
    proj_pj = pj_init_plus(definition.c_str());
}

Projection::~Projection() {
    pj_free(this->proj_pj);
}

navitia::type::GeographicalCoord ConvCoord::convert_to(navitia::type::GeographicalCoord coord) const{
    if(this->origin.is_degree){
        coord.set_lon(coord.lon() * DEG_TO_RAD);
        coord.set_lat(coord.lat() * DEG_TO_RAD);
    }
    double x = coord.lon();
    double y = coord.lat();
    pj_transform(this->origin.proj_pj, this->destination.proj_pj, 1, 1,&x, &y, NULL);
    coord.set_lon(x);
    coord.set_lat(y);
    if(this->destination.is_degree){
        coord.set_lon(coord.lon() * RAD_TO_DEG);
        coord.set_lat(coord.lat() * RAD_TO_DEG);
    }
    return coord;
}

}//namespace connectors
}//namespace ed
