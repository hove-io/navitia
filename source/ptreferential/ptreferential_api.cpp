#include "ptreferential.h"
#include "type/pb_converter.h"
#include "type/data.h"

namespace pt = boost::posix_time;

namespace navitia{ namespace ptref{

pbnavitia::Response extract_data(const type::Data & data, type::Type_e requested_type, std::vector<type::idx_t> & rows, const int depth) {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_response = result.mutable_ptref();

    //on utilise la date courante pour choisir si on doit afficher les  messages de perturbation
    pt::ptime today = pt::second_clock::local_time();

    for(type::idx_t idx : rows){
        switch(requested_type){
        case type::Type_e::eLine: fill_pb_object(idx, data, pb_response->add_lines(), depth, today); break;
        case type::Type_e::eJourneyPattern: fill_pb_object(idx, data, pb_response->add_journey_patterns(), depth, today); break;
        case type::Type_e::eStopPoint: fill_pb_object(idx, data, pb_response->add_stop_points(), depth, today); break;
        case type::Type_e::eStopArea: fill_pb_object(idx, data, pb_response->add_stop_areas(), depth, today); break;
        case type::Type_e::eNetwork: fill_pb_object(idx, data, pb_response->add_networks(), depth); break;
        case type::Type_e::ePhysicalMode: fill_pb_object(idx, data, pb_response->add_physical_modes(), depth); break;
        case type::Type_e::eCommercialMode: fill_pb_object(idx, data, pb_response->add_commercial_modes(), depth); break;
        case type::Type_e::eCity: fill_pb_object(idx, data, pb_response->add_cities(), depth); break;
        case type::Type_e::eConnection: fill_pb_object(idx, data, pb_response->add_connections(), depth); break;
        case type::Type_e::eJourneyPatternPoint: fill_pb_object(idx, data, pb_response->add_journey_pattern_points(), depth, today); break;
        case type::Type_e::eCompany: fill_pb_object(idx, data, pb_response->add_companies(), depth); break;
        case type::Type_e::eVehicleJourney: fill_pb_object(idx, data, pb_response->add_vehicle_journeys(), depth, today); break;
        case type::Type_e::eRoute: fill_pb_object(idx, data, pb_response->add_routes(), depth, today); break;
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
    } catch(parsing_error parse_error) {
        pb_response.set_error(parse_error.more);
        return pb_response;
    }

    return extract_data(data, requested_type, final_indexes, depth);
}


}}
