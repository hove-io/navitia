#include "worker.h"

#include "utils/configuration.h"
#include "routing/raptor_api.h"
#include "autocomplete/autocomplete_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential_api.h"
#include "time_tables/route_schedules.h"
#include "time_tables/next_passages.h"
#include "time_tables/2stops_schedules.h"
#include "time_tables/departure_boards.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

namespace navitia {

nt::Type_e get_type(pbnavitia::NavitiaType pb_type){
    switch(pb_type){
    case pbnavitia::ADDRESS: return nt::Type_e::Address; break;
    case pbnavitia::STOP_AREA: return nt::Type_e::StopArea; break;
    case pbnavitia::STOP_POINT: return nt::Type_e::StopPoint; break;
    case pbnavitia::LINE: return nt::Type_e::Line; break;
    case pbnavitia::ROUTE: return nt::Type_e::Route; break;
    case pbnavitia::JOURNEY_PATTERN: return nt::Type_e::JourneyPattern; break;
    case pbnavitia::NETWORK: return nt::Type_e::Network; break;
    case pbnavitia::COMMERCIAL_MODE: return nt::Type_e::CommercialMode; break;
    case pbnavitia::PHYSICAL_MODE: return nt::Type_e::PhysicalMode; break;
    case pbnavitia::CONNECTION: return nt::Type_e::Connection; break;
    case pbnavitia::JOURNEY_PATTERN_POINT: return nt::Type_e::JourneyPatternPoint; break;
    case pbnavitia::COMPANY: return nt::Type_e::Company; break;
    case pbnavitia::VEHICLE_JOURNEY: return nt::Type_e::VehicleJourney; break;
	case pbnavitia::POI: return nt::Type_e::POI; break;
    case pbnavitia::ADMIN: return nt::Type_e::Admin; break;
    default: return nt::Type_e::Unknown;
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

template<class T>
std::vector<std::string> vector_of_admins(const T & admin){
    std::vector<std::string> result;
    for (int i = 0; i < admin.admin_uris_size(); ++i){
        result.push_back(admin.admin_uris(i));
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
    return navitia::autocomplete::autocomplete(request.name(), vector_of_pb_types(request), request.depth(), request.nbmax(), vector_of_admins(request), this->data);
}

pbnavitia::Response Worker::next_stop_times(const pbnavitia::NextStopTimeRequest & request, pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    this->init_worker_data();
    switch(api){
    case pbnavitia::NEXT_DEPARTURES:
        return navitia::timetables::next_departures(request.departure_filter(), request.from_datetime(), request.duration(), request.nb_stoptimes(), request.depth(),/* request.wheelchair()*/false, this->data);
    case pbnavitia::NEXT_ARRIVALS:
        return navitia::timetables::next_arrivals(request.arrival_filter(), request.from_datetime(), request.duration(), request.nb_stoptimes(), request.depth(), /*request.wheelchair()*/false, this->data);
    case pbnavitia::STOPS_SCHEDULES:
        return navitia::timetables::stops_schedule(request.departure_filter(), request.arrival_filter(), request.from_datetime(), request.duration(), request.depth(), this->data);
    case pbnavitia::DEPARTURE_BOARDS:
        return navitia::timetables::departure_board(request.departure_filter(), request.from_datetime(), request.duration(), this->data);
    case pbnavitia::ROUTE_SCHEDULES:
        return navitia::timetables::route_schedule(request.departure_filter(), request.from_datetime(), request.duration(), request.depth(), this->data);
    default:
        LOG4CPLUS_WARN(logger, "On a reçu une requête time table inconnue");
        pbnavitia::Response response;
        response.set_error("Unknown time table api");
        return response;
    }
}

pbnavitia::Response Worker::proximity_list(const pbnavitia::ProximityListRequest &request) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    type::EntryPoint ep(request.uri());
    auto coord = this->coord_of_entry_point(ep);
    return navitia::proximitylist::find(coord, request.distance(), vector_of_pb_types(request), request.depth(), this->data);
}

type::GeographicalCoord Worker::coord_of_entry_point(const type::EntryPoint & entry_point) {
    type::GeographicalCoord result;
    if(entry_point.type == Type_e::Address){
        auto way = this->data.geo_ref.way_map.find(entry_point.uri);
        if (way != this->data.geo_ref.way_map.end()){
            result = this->data.geo_ref.ways[way->second].nearest_coord(entry_point.house_number, this->data.geo_ref.graph);
        }
    } else if (entry_point.type == Type_e::StopPoint) {
        auto sp_it = this->data.pt_data.stop_points_map.find(entry_point.uri);
        if(sp_it != this->data.pt_data.stop_points_map.end()) {
            result = this->data.pt_data.stop_points[sp_it->second].coord;
        }
    } else if (entry_point.type == Type_e::StopArea) {
           auto sp_it = this->data.pt_data.stop_areas_map.find(entry_point.uri);
           if(sp_it != this->data.pt_data.stop_areas_map.end()) {
               result = this->data.pt_data.stop_areas[sp_it->second].coord;
           }
    } else if (entry_point.type == Type_e::Coord) {
        result = entry_point.coordinates;
    }
    return result;
}

pbnavitia::Response Worker::journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock(data.load_mutex);
    this->init_worker_data();

    type::EntryPoint origin = type::EntryPoint(request.origin());

    if (origin.type == type::Type_e::Address) {
        origin.coordinates = this->coord_of_entry_point(origin);
    }

    type::EntryPoint destination;
    if(api != pbnavitia::ISOCHRONE) {
        destination = type::EntryPoint(request.destination());
        if (destination.type == type::Type_e::Address) {
            destination.coordinates = this->coord_of_entry_point(destination);
        }
    }

    std::vector<std::string> forbidden;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden.push_back(request.forbidden_uris(i));

    std::vector<std::string> datetimes;
    for(int i = 0; i < request.datetimes_size(); ++i)
        datetimes.push_back(request.datetimes(i));


    if(api != pbnavitia::ISOCHRONE){
        return routing::raptor::make_response(*calculateur, origin, destination, datetimes,
                                              request.clockwise(), request.walking_speed(), request.walking_distance(), /*request.wheelchair()*/false,
                                              forbidden, *street_network_worker);
    } else {
        return navitia::routing::raptor::make_isochrone(*calculateur, origin, request.datetimes(0),
                                                        request.clockwise(), request.walking_speed(), request.walking_distance(), /*request.wheelchair()*/false,
                                                        forbidden, *street_network_worker, request.max_duration());
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
    case pbnavitia::ROUTE_SCHEDULES:
    case pbnavitia::NEXT_DEPARTURES:
    case pbnavitia::NEXT_ARRIVALS:
    case pbnavitia::STOPS_SCHEDULES:
    case pbnavitia::DEPARTURE_BOARDS:
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
