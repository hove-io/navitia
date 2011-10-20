#include "type.h"
#include "type.pb.h"

namespace nt = navitia::type;
namespace navitia{



/**
 * fonction générique pour convertire un objet navitia en un message protocol buffer
 *
 * @param idx identifiant de l'objet à convertire
 * @param data reférence vers l'objet Data de l'application
 * @param message l'objet protocol buffer a remplire 
 * @param depth profondeur de remplissage
 *
 * @throw std::out_of_range si l'idx n'est pas valide
 * @throw std::bad_cast si le message PB n'est pas adapté
 */
template<nt::Type_e type>
void fill_pb_object(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth = 0){throw std::exception();}

template<>
void fill_pb_object<nt::eCity>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::City* city = dynamic_cast<pbnavitia::City*>(message);
    nt::City city_n = data.cities.at(idx);
    city->set_id(city_n.id);
    city->set_id(city_n.id);
    city->set_idx(city_n.idx);
    city->set_external_code(city_n.external_code);
    city->set_name(city_n.name);
    city->mutable_coord()->set_x(city_n.coord.x);
    city->mutable_coord()->set_y(city_n.coord.y);
}

/**
 * spécialisation de fill_pb_object pour les StopArea
 *
 */
template<>
void fill_pb_object<nt::eStopArea>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::StopArea* stop_area = dynamic_cast<pbnavitia::StopArea*>(message);
    nt::StopArea sa = data.stop_areas.at(idx);
    stop_area->set_id(sa.id);
    stop_area->set_idx(sa.idx);
    stop_area->set_external_code(sa.external_code);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_x(sa.coord.x);
    stop_area->mutable_coord()->set_y(sa.coord.y);
    if(max_depth > 0){
        try{
            fill_pb_object<nt::eCity>(sa.city_idx, data, stop_area->mutable_child()->add_city_list(), max_depth-1);
        }catch(std::out_of_range e){}
    }
}

}//namespace navitia
