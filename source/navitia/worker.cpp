#include "worker.h"

#include "utils/configuration.h"
#include "routing/raptor_api.h"
#include "autocomplete/autocomplete_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential_api.h"
#include "time_tables/line_schedule.h"
#include "time_tables/next_stop_times.h"
#include "time_tables/2stops_schedule.h"
#include "time_tables/departure_board.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

namespace navitia {

nt::Type_e get_type(pbnavitia::NavitiaType pb_type){
    switch(pb_type){
    case pbnavitia::ADDRESS: return nt::Type_e::eAddress; break;
    case pbnavitia::STOP_AREA: return nt::Type_e::eStopArea; break;
    case pbnavitia::STOP_POINT: return nt::Type_e::eStopPoint; break;
    case pbnavitia::CITY: return nt::Type_e::eCity; break;
    case pbnavitia::LINE: return nt::Type_e::eLine; break;
    case pbnavitia::ROUTE: return nt::Type_e::eRoute; break;
    case pbnavitia::NETWORK: return nt::Type_e::eNetwork; break;
    case pbnavitia::COMMERCIAL_MODE: return nt::Type_e::ePhysicalMode; break;
    case pbnavitia::PHYSICAL_MODE: return nt::Type_e::eCommercialMode; break;
    case pbnavitia::CONNECTION: return nt::Type_e::eConnection; break;
    case pbnavitia::ROUTE_POINT: return nt::Type_e::eRoutePoint; break;
    case pbnavitia::COMPANY: return nt::Type_e::eCompany; break;
    case pbnavitia::VEHICLE_JOURNEY: return nt::Type_e::eVehicleJourney; break;
    default: return nt::Type_e::eUnknown;
    }
}

template<class T>
std::vector<nt::Type_e> vector_of_pb_types(const T & pb_object){
    std::vector<nt::Type_e> result;
    for(int i = 0; i < pb_object.types_size(); ++i){
        result.push_back(get_type(pb_object.types(i)));
    }
    return result;
}

pbnavitia::Response Worker::status() {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::STATUS);

    auto status = result.mutable_status();

    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    status->set_publication_date(pt::to_iso_string(data.meta.publication_date));
    status->set_start_production_date(bg::to_iso_string(data.meta.production_date.begin()));
    status->set_end_production_date(bg::to_iso_string(data.meta.production_date.end()));

    for(auto data_sources: data.meta.data_sources){
        status->add_data_sources(data_sources);
    }
    status->set_data_version(data.version);
    status->set_navimake_version(data.meta.navimake_version);
    status->set_navitia_version(NAVITIA_VERSION);

    status->set_loaded(data.loaded);
    status->set_last_load_status(data.last_load);
    status->set_last_load_at(pt::to_iso_string(data.last_load_at));
    status->set_nb_threads(data.nb_threads);

    return result;
}

pbnavitia::Response Worker::metadatas() {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::METADATAS);

    auto metadatas = result.mutable_metadatas();

    metadatas->set_start_production_date(bg::to_iso_string(data.meta.production_date.begin()));
    metadatas->set_end_production_date(bg::to_iso_string(data.meta.production_date.end()));
    metadatas->set_shape(data.meta.shape);
    metadatas->set_status("running");

    return result;
}

void Worker::init_worker_data(){
    if(this->data.last_load_at != this->last_load_at || !calculateur){
        calculateur = std::unique_ptr<navitia::routing::raptor::RAPTOR>(new navitia::routing::raptor::RAPTOR(this->data));
        street_network_worker = std::unique_ptr<navitia::streetnetwork::StreetNetwork>(new navitia::streetnetwork::StreetNetwork(this->data.geo_ref));
        this->last_load_at = this->data.last_load_at;

        LOG4CPLUS_INFO(logger, "instanciation du calculateur");
    }
}

pbnavitia::Response Worker::load() {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::LOAD);
    data.to_load = true;
    result.mutable_load()->set_ok(true);

    return result;
}



pbnavitia::Response Worker::autocomplete(const pbnavitia::AutocompleteRequest & request) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    return navitia::autocomplete::autocomplete(request.name(), vector_of_pb_types(request), this->data);
}

pbnavitia::Response Worker::next_stop_times(const pbnavitia::NextStopTimeRequest & request, pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    this->init_worker_data();
    switch(api){
    case pbnavitia::NEXT_DEPARTURES:
        return navitia::timetables::next_departures(request.departure_filter(), request.from_datetime(), request.duration(), request.nb_stoptimes(), request.depth(), request.wheelchair(), this->data);
    case pbnavitia::NEXT_ARRIVALS:
        return navitia::timetables::next_arrivals(request.arrival_filter(), request.from_datetime(), request.duration(), request.nb_stoptimes(), request.depth(), request.wheelchair(), this->data);
    case pbnavitia::STOPS_SCHEDULE:
        return navitia::timetables::stops_schedule(request.departure_filter(), request.arrival_filter(), request.from_datetime(), request.duration(), request.nb_stoptimes(), request.depth(), this->data);
    case pbnavitia::DEPARTURE_BOARD:
        return navitia::timetables::departure_board(request.departure_filter(), request.from_datetime(), request.duration(), this->data);
    default:
        return navitia::timetables::line_schedule(request.departure_filter(), request.from_datetime(), request.duration(), request.depth(), this->data);
    }
}

pbnavitia::Response Worker::proximity_list(const pbnavitia::ProximityListRequest &request) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    return navitia::proximitylist::find(type::GeographicalCoord(request.coord().lon(), request.coord().lat()), request.distance(), vector_of_pb_types(request), this->data);
}

type::GeographicalCoord Worker::coord_of_address(const type::EntryPoint & entry_point) {
    type::GeographicalCoord result;
    auto way = this->data.geo_ref.way_map.find(entry_point.uri);
    if (way != this->data.geo_ref.way_map.end()){
        result = this->data.geo_ref.ways[way->second].nearest_coord(entry_point.house_number, this->data.geo_ref.graph);
    }
    return result;
}

pbnavitia::Response Worker::journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    this->init_worker_data();

    type::EntryPoint origin = type::EntryPoint(request.origin());

    if (origin.type == type::Type_e::eAddress) {
        origin.coordinates = this->coord_of_address(origin);
    }

    type::EntryPoint destination;
    if(api != pbnavitia::ISOCHRONE) {
        destination = type::EntryPoint(request.destination());
        if (destination.type == type::Type_e::eAddress) {
            destination.coordinates = this->coord_of_address(destination);
        }
    }

    std::multimap<std::string, std::string> forbidden;
    for(int i = 0; i < request.forbiddenlines_size(); ++i)
        forbidden.insert(std::make_pair("line", request.forbiddenlines(i)));
    for(int i = 0; i < request.forbiddenmodes_size(); ++i)
        forbidden.insert(std::make_pair("mode", request.forbiddenmodes(i)));
    for(int i = 0; i < request.forbiddenroutes_size(); ++i)
        forbidden.insert(std::make_pair("route", request.forbiddenroutes(i)));

    std::vector<std::string> datetimes;
    for(int i = 0; i < request.datetimes_size(); ++i)
        datetimes.push_back(request.datetimes(i));


    if(api != pbnavitia::ISOCHRONE){
        return routing::raptor::make_response(*calculateur, origin, destination, datetimes,
                                              request.clockwise(), request.walking_speed(), request.walking_distance(), request.wheelchair(),
                                              forbidden, *street_network_worker);
    } else {
        return navitia::routing::raptor::make_isochrone(*calculateur, origin, request.datetimes(0),
                                                        request.clockwise(), request.walking_speed(), request.walking_distance(), request.wheelchair(),
                                                        forbidden, *street_network_worker);
    }
}

pbnavitia::Response Worker::pt_ref(const pbnavitia::PTRefRequest &request){
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    return navitia::ptref::query_pb(get_type(request.requested_type()), request.filter(), request.depth(), this->data);
}


pbnavitia::Response Worker::dispatch(const pbnavitia::Request & request) {
    pbnavitia::Response result;
    result.set_requested_api(request.requested_api());
    switch(request.requested_api()){
    case pbnavitia::STATUS: return status(); break;
    case pbnavitia::LOAD: return load(); break;
    case pbnavitia::AUTOCOMPLETE: return autocomplete(request.autocomplete()); break;
    case pbnavitia::LINE_SCHEDULE:
    case pbnavitia::NEXT_DEPARTURES:
    case pbnavitia::NEXT_ARRIVALS:
    case pbnavitia::STOPS_SCHEDULE:
    case pbnavitia::DEPARTURE_BOARD:
        return next_stop_times(request.next_stop_times(), request.requested_api()); break;
    case pbnavitia::ISOCHRONE:
    case pbnavitia::PLANNER: return journeys(request.journeys(), request.requested_api()); break;
    case pbnavitia::PROXIMITY_LIST: return proximity_list(request.proximity_list()); break;
    case pbnavitia::PTREFERENTIAL: return pt_ref(request.ptref()); break;
    case pbnavitia::METADATAS : return metadatas(); break;
    default: break;
    }

    return result;
}

}
