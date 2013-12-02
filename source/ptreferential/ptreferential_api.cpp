#include "ptreferential.h"
#include "type/pb_converter.h"
#include "type/data.h"

namespace pt = boost::posix_time;

namespace navitia{ namespace ptref{

pbnavitia::Response extract_data(const type::Data & data,
                                 type::Type_e requested_type,
                                 std::vector<type::idx_t> & rows,
                                 const int depth) {
    pbnavitia::Response result;
    //on utilise la date courante pour choisir si on doit afficher les  messages de perturbation
    pt::ptime today = pt::second_clock::local_time();
    for(auto idx : rows){
        switch(requested_type){
        case Type_e::ValidityPattern:
            fill_pb_object(data.pt_data.validity_patterns[idx], data,
                           result.add_validity_patterns(), depth, today);
            break;
        case Type_e::Line:
            fill_pb_object(data.pt_data.lines[idx], data, result.add_lines(),
                           depth, today);
            break;
        case Type_e::JourneyPattern:
            fill_pb_object(data.pt_data.journey_patterns[idx], data,
                           result.add_journey_patterns(), depth, today);
            break;
        case Type_e::StopPoint:
            fill_pb_object(data.pt_data.stop_points[idx], data,
                           result.add_stop_points(), depth, today);
            break;
        case Type_e::StopArea:
            fill_pb_object(data.pt_data.stop_areas[idx], data,
                           result.add_stop_areas(), depth, today);
            break;
        case Type_e::Network:
            fill_pb_object(data.pt_data.networks[idx], data,
                           result.add_networks(), depth, today);
            break;
        case Type_e::PhysicalMode:
            fill_pb_object(data.pt_data.physical_modes[idx], data,
                           result.add_physical_modes(), depth, today);
            break;
        case Type_e::CommercialMode:
            fill_pb_object(data.pt_data.commercial_modes[idx], data,
                           result.add_commercial_modes(), depth, today);
            break;
        case Type_e::JourneyPatternPoint:
            fill_pb_object(data.pt_data.journey_pattern_points[idx], data,
                           result.add_journey_pattern_points(), depth, today);
            break;
        case Type_e::Company:
            fill_pb_object(data.pt_data.companies[idx], data,
                           result.add_companies(), depth, today);
            break;
        case Type_e::Route:
            fill_pb_object(data.pt_data.routes[idx], data,
                    result.add_routes(), depth, today);
            break;
        case Type_e::POI:
            fill_pb_object(data.geo_ref.pois[idx], data,
                           result.add_pois(), depth, today);
            break;
        case Type_e::POIType:
            fill_pb_object(data.geo_ref.poitypes[idx], data,
                           result.add_poi_types(), depth, today);
            break;
        case Type_e::Connection:
            fill_pb_object(data.pt_data.stop_point_connections[idx], data,
                           result.add_connections(), depth, today);
            break;
        case Type_e::VehicleJourney:
            fill_pb_object(data.pt_data.vehicle_journeys[idx], data,
                           result.add_vehicle_journeys(), depth, today);
            break;
        default: break;
        }
    }
    return result;
}


pbnavitia::Response query_pb(type::Type_e requested_type, std::string request,
                             const std::vector<std::string>& forbidden_uris,
                             const int depth, const int startPage,
                             const int count, const type::Data &data){
    std::vector<type::idx_t> final_indexes;
    pbnavitia::Response pb_response;
    int total_result;
    try {
        final_indexes = make_query(requested_type, request, forbidden_uris, data);
        total_result = final_indexes.size();
        final_indexes = paginate(final_indexes, count, startPage);
    } catch(const parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse :" + parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref_error &pt_error) {
        fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + pt_error.more, pb_response.mutable_error());
        return pb_response;
    }

    pb_response = extract_data(data, requested_type, final_indexes, depth);
    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(startPage);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(final_indexes.size());
    return pb_response;
}


}}
