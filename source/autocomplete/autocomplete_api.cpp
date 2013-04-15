#include "autocomplete_api.h"
#include "type/pb_converter.h"

namespace navitia { namespace autocomplete {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
void create_pb(const std::vector<Autocomplete<nt::idx_t>::fl_quality>& result,
               const nt::Type_e type, uint32_t depth, const nt::Data& data,
               pbnavitia::Autocomplete& pb_fl){
    for(auto result_item : result){
        pbnavitia::AutocompleteItem* item = pb_fl.add_items();
        pbnavitia::PlaceMark* place_mark = item->mutable_object();
        switch(type){
        case nt::Type_e::StopArea:
            place_mark->set_type(pbnavitia::STOP_AREA);
            fill_pb_object(result_item.idx, data, place_mark->mutable_stop_area(), depth);
            item->set_name(data.pt_data.stop_areas[result_item.idx].name);
            item->set_uri(data.pt_data.stop_areas[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::Admin:
            place_mark->set_type(pbnavitia::ADMIN);
            fill_pb_object(result_item.idx, data, place_mark->mutable_admin(), depth);
            item->set_quality(result_item.quality);
            item->set_uri(data.geo_ref.admins[result_item.idx].uri);
            item->set_name(data.geo_ref.admins[result_item.idx].name);
            break;
        case nt::Type_e::StopPoint:
            place_mark->set_type(pbnavitia::STOP_POINT);
            fill_pb_object(result_item.idx, data, place_mark->mutable_stop_point(), depth);
            item->set_name(data.pt_data.stop_points[result_item.idx].name);
            item->set_uri(data.pt_data.stop_points[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::Address:
            place_mark->set_type(pbnavitia::ADDRESS);
            fill_pb_object(result_item.idx, data, place_mark->mutable_address(), result_item.house_number,result_item.coord, depth);
            item->set_name(data.geo_ref.ways[result_item.idx].name);
            item->set_uri(data.geo_ref.ways[result_item.idx].uri+":"+boost::lexical_cast<std::string>(result_item.house_number));
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::POI:
            place_mark->set_type(pbnavitia::POI);
            fill_pb_object(result_item.idx, data, place_mark->mutable_poi(), depth);
            item->set_name(data.geo_ref.pois[result_item.idx].name);
            item->set_uri(data.geo_ref.pois[result_item.idx].uri);
            item->set_quality(result_item.quality);
            break;
        case nt::Type_e::Line:
            place_mark->set_type(pbnavitia::LINE);
            fill_pb_object(result_item.idx, data, place_mark->mutable_line(), depth);
            item->set_name(data.pt_data.lines[result_item.idx].name);
            item->set_uri(data.pt_data.lines[result_item.idx].uri);
            item->set_quality(result_item.quality);
        default:
            break;
        }
    }
}

int penalty_by_type(navitia::type::Type_e ntype, bool Is_address_type) {
    // Ordre de tri :
    // Add, SA, POI, SP, City : si présence de addressType dans le recherche
    // City, SA, POI, Add, SP : si non
    int result = 0;
    switch(ntype){
    case navitia::type::Type_e::Admin:
        result = Is_address_type ? 8 : 0;
        break;
    case navitia::type::Type_e::StopArea:
        result = 2;
        break;
    case navitia::type::Type_e::POI:
        result = 4;
        break;
    case navitia::type::Type_e::Address:
        result = Is_address_type ? 0 : 6;
        break;
    case navitia::type::Type_e::StopPoint:
        result = 8;
        result = Is_address_type ? 6 : 8;
        break;
    default:
        break;
    }
    return result;
}

void Update_quality(std::vector<Autocomplete<nt::idx_t>::fl_quality>& ac_result, navitia::type::Type_e ntype, bool Is_address_type){
    //Mettre à jour la qualité sur la pénalité par type
    int penalty = penalty_by_type(ntype, Is_address_type);
    for(auto &item : ac_result){
        item.quality -= penalty;
    }
}


pbnavitia::Response autocomplete(const std::string &name,
                                 const std::vector<nt::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const navitia::type::Data &d){

    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::AUTOCOMPLETE);
    bool addType = d.pt_data.stop_area_autocomplete.is_address_type(name, d.geo_ref.alias, d.geo_ref.synonymes);
    std::vector<Autocomplete<nt::idx_t>::fl_quality> result;
    pbnavitia::Autocomplete* pb = pb_response.mutable_autocomplete();
    for(nt::Type_e type : filter){
        switch(type){
        case nt::Type_e::StopArea:
            result = d.pt_data.stop_area_autocomplete.find_complete(name, d.geo_ref.alias, d.geo_ref.synonymes, d.geo_ref.word_weight, nbmax);
            break;
        case nt::Type_e::StopPoint:
            result = d.pt_data.stop_point_autocomplete.find_complete(name, d.geo_ref.alias, d.geo_ref.synonymes, d.geo_ref.word_weight, nbmax);
            break;
        case nt::Type_e::Admin:
            result = d.geo_ref.fl_admin.find_complete(name, d.geo_ref.alias, d.geo_ref.synonymes, d.geo_ref.word_weight, nbmax);
            break;
        case nt::Type_e::Address:
            result = d.geo_ref.find_ways(name, nbmax);
            break;
        case nt::Type_e::POI:
            result = d.geo_ref.fl_poi.find_complete(name, d.geo_ref.alias, d.geo_ref.synonymes, d.geo_ref.word_weight, nbmax);
            break;
        case nt::Type_e::Line:
            result = d.pt_data.line_autocomplete.find_complete(name, d.geo_ref.alias, d.geo_ref.synonymes, d.geo_ref.word_weight, nbmax);
            break;
        default: break;
        }

        Update_quality(result, type, addType);

        create_pb(result, type, depth, d, *pb);
    }

    auto compare = [](pbnavitia::AutocompleteItem a, pbnavitia::AutocompleteItem b){
            return a.quality() > b.quality();
    };

    //Trier la partiallement jusqu'au nbmax elément.
    int result_size = std::min(nbmax, pb_response.mutable_autocomplete()->mutable_items()->size());
    std::partial_sort(pb_response.mutable_autocomplete()->mutable_items()->begin(),pb_response.mutable_autocomplete()->mutable_items()->begin() + result_size,
                      pb_response.mutable_autocomplete()->mutable_items()->end(),compare);

    while (pb_response.mutable_autocomplete()->mutable_items()->size() > nbmax){
        pb_response.mutable_autocomplete()->mutable_items()->RemoveLast();
    }
    return pb_response;
}

}} //namespace navitia::autocomplete
