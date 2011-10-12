#include "type.h"
#include "type.pb.h"

namespace navitia{
namespace nt = navitia::type;

template<nt::Type_e type>
void fill_pb_object(nt::idx_t idx, const nt::Data&, google::protobuf::Message* message){
}

template<>
void fill_pb_object<nt::eStopArea>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message){
    pbnavitia::StopArea* stop_area = dynamic_cast<pbnavitia::StopArea*>(message);
    nt::StopArea sa = data.stop_areas[idx];
    stop_area->set_id(sa.id);
    stop_area->set_idx(sa.idx);
    stop_area->set_external_code(sa.external_code);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_x(sa.coord.x);
    stop_area->mutable_coord()->set_y(sa.coord.y);
}

template<>
void fill_pb_object<nt::eCity>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message){
    pbnavitia::City* city = dynamic_cast<pbnavitia::City*>(message);
    nt::City city_n = data.cities[idx];
    city->set_id(city_n.id);
    city->set_id(city_n.id);
    city->set_idx(city_n.idx);
    city->set_external_code(city_n.external_code);
    city->set_name(city_n.name);
    city->mutable_coord()->set_x(city_n.coord.x);
    city->mutable_coord()->set_y(city_n.coord.y);
}

}//namespace navitia
