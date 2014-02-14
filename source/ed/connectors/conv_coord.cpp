#include "conv_coord.h"

namespace ed{ namespace connectors{

Projection::Projection(const std::string& name, const std::string& num_epsg, bool is_degree): name(name), is_degree(is_degree){
    this->definition = "+init=epsg:" + num_epsg;
}

ConvCoord::ConvCoord(const Projection& origin, const Projection& destination): origin(origin), destination(destination){
    this->pj_origin = pj_init_plus(this->origin.definition.c_str());
    this->pj_destination = pj_init_plus(this->destination.definition.c_str());
}

ConvCoord::~ConvCoord(){
    pj_free(this->pj_destination);
    pj_free(this->pj_origin);
}

navitia::type::GeographicalCoord ConvCoord::convert_to(navitia::type::GeographicalCoord coord) const{

    if(this->origin.is_degree){
        coord.set_lon(coord.lon() * DEG_TO_RAD);
        coord.set_lat(coord.lat() * DEG_TO_RAD);
    }
    double x = coord.lon();
    double y = coord.lat();
    pj_transform(this->pj_origin, this->pj_destination, 1, 1,&x, &y, NULL);
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
