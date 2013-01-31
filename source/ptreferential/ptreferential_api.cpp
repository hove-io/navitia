#include "ptreferential.h"
#include "type/pb_converter.h"
#include "type/data.h"
namespace navitia{ namespace ptref{

pbnavitia::Response extract_data(const type::Data & data, type::Type_e requested_type, std::vector<type::idx_t> & rows, const int depth) {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_response = result.mutable_ptref();

    for(type::idx_t idx : rows){
        switch(requested_type){
        case type::Type_e::eLine: fill_pb_object(idx, data, pb_response->add_line(), depth); break;
        case type::Type_e::eRoute: fill_pb_object(idx, data, pb_response->add_route(), depth); break;
        case type::Type_e::eStopPoint: fill_pb_object(idx, data, pb_response->add_stop_point(), depth); break;
        case type::Type_e::eStopArea: fill_pb_object(idx, data, pb_response->add_stop_area(), depth); break;
        case type::Type_e::eNetwork: fill_pb_object(idx, data, pb_response->add_network(), depth); break;
        case type::Type_e::eMode: fill_pb_object(idx, data, pb_response->add_commercial_mode(), depth); break;
        case type::Type_e::eModeType: fill_pb_object(idx, data, pb_response->add_physical_mode(), depth); break;
        case type::Type_e::eCity: fill_pb_object(idx, data, pb_response->add_city(), depth); break;
        case type::Type_e::eConnection: fill_pb_object(idx, data, pb_response->add_connection(), depth); break;
        case type::Type_e::eRoutePoint: fill_pb_object(idx, data, pb_response->add_route_point(), depth); break;
        case type::Type_e::eCompany: fill_pb_object(idx, data, pb_response->add_company(), depth); break;
        case type::Type_e::eVehicleJourney: fill_pb_object(idx, data, pb_response->add_vehicle_journey(), depth); break;
        default: break;
        }
    }
    return result;
}

pbnavitia::Response query_pb(type::Type_e requested_type, std::string request, const int depth, const type::Data &data){
    std::vector<type::idx_t> final_indexes;
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PTREFERENTIAL);
    try {
        final_indexes = make_query(requested_type, request, data);
    } catch(ptref_parsing_error parse_error) {
        switch(parse_error.type){
        case ptref_parsing_error::error_type::partial_error:
            pb_response.set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<");
            break;
        case ptref_parsing_error::error_type::unknown_object:
            pb_response.set_error("Objet NAViTiA inconnu : " + parse_error.more);
            break;
        case ptref_parsing_error::error_type::global_error:
            pb_response.set_error("PTReferential : Impossible de parser la requête");
            break;
        }

        return pb_response;
    }

    //final_indexes = make_query(requested_type, request, data);
    return extract_data(data, requested_type, final_indexes, depth);
}


}}
