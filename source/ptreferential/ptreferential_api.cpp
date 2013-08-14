#include "ptreferential.h"
#include "type/pb_converter.h"
#include "type/data.h"

namespace pt = boost::posix_time;

namespace navitia{ namespace ptref{

pbnavitia::Response extract_data(const type::Data & data,
                                 type::Type_e requested_type,
                                 std::vector<type::idx_t> & rows,
                                 const int startPage, const int count,
                                 const int depth) {
    pbnavitia::Response result;

    //on utilise la date courante pour choisir si on doit afficher les  messages de perturbation
    pt::ptime today = pt::second_clock::local_time();
    uint32_t begin_i = startPage * count;
    uint32_t end_i = begin_i + count;
    if(begin_i < rows.size()) {
        auto begin = rows.begin() + begin_i;
        if(end_i >= rows.size()) {
            end_i = rows.size();
        }
        auto end = rows.begin() + end_i;
        for(auto it_idx=begin; it_idx < end; ++it_idx){
            type::idx_t idx = *it_idx;
            switch(requested_type){
            case Type_e::ValidityPattern: fill_pb_object(data.pt_data.validity_patterns[idx], data, result.add_validity_patterns(), depth, today); break;
            case Type_e::Line: fill_pb_object(data.pt_data.lines[idx], data, result.add_lines(), depth, today); break;
            case Type_e::JourneyPattern: fill_pb_object(data.pt_data.journey_patterns[idx], data, result.add_journey_patterns(), depth, today); break;
            case Type_e::StopPoint: fill_pb_object(data.pt_data.stop_points[idx], data, result.add_stop_points(), depth, today); break;
            case Type_e::StopArea: fill_pb_object(data.pt_data.stop_areas[idx], data, result.add_stop_areas(), depth, today); break;
            case Type_e::Network: fill_pb_object(data.pt_data.networks[idx], data, result.add_networks(), depth, today); break;
            case Type_e::PhysicalMode: fill_pb_object(data.pt_data.physical_modes[idx], data, result.add_physical_modes(), depth, today); break;
            case Type_e::CommercialMode: fill_pb_object(data.pt_data.commercial_modes[idx], data, result.add_commercial_modes(), depth, today); break;
            case Type_e::JourneyPatternPoint: fill_pb_object(data.pt_data.journey_pattern_points[idx], data, result.add_journey_pattern_points(), depth, today); break;
            case Type_e::Company: fill_pb_object(data.pt_data.companies[idx], data, result.add_companies(), depth, today); break;
            case Type_e::Route: fill_pb_object(data.pt_data.routes[idx], data, result.add_routes(), depth, today); break;
            case Type_e::POI: fill_pb_object(data.geo_ref.pois[idx], data, result.add_pois(), depth, today); break;
            case Type_e::POIType: fill_pb_object(data.geo_ref.poitypes[idx], data, result.add_poi_types(), depth, today); break;
            default: break;
            }
        }

    }
    auto pagination = result.mutable_pagination();
    pagination->set_totalresult(rows.size());
    pagination->set_startpage(startPage);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(end_i - begin_i);
    return result;
}

pbnavitia::Response query_pb(type::Type_e requested_type, std::string request,
                             const int depth, const int startPage,
                             const int count, const type::Data &data){
    std::vector<type::idx_t> final_indexes;
    pbnavitia::Response pb_response;
    try {
        final_indexes = make_query(requested_type, request, data);
    } catch(const parsing_error &parse_error) {
        pb_response.set_error(parse_error.more);
        return pb_response;
    } catch(const ptref_error &pt_error) {
        pb_response.set_error(pt_error.more);
        return pb_response;
    }

    return extract_data(data, requested_type, final_indexes, startPage, count, depth);
}


}}
