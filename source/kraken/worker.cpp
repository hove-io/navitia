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
    case pbnavitia::POITYPE: return nt::Type_e::POIType; break;
    case pbnavitia::ADMINISTRATIVE_REGION: return nt::Type_e::Admin; break;
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

    auto status = result.mutable_status();
    const auto d = *data;
    boost::shared_lock<boost::shared_mutex> lock(d->load_mutex);
    status->set_publication_date(pt::to_iso_string(d->meta.publication_date));
    status->set_start_production_date(bg::to_iso_string(d->meta.production_date.begin()));
    status->set_end_production_date(bg::to_iso_string(d->meta.production_date.end()));
    status->set_data_version(d->version);
    status->set_navimake_version(d->meta.navimake_version);
    status->set_navitia_version(NAVITIA_VERSION);
    status->set_loaded(d->loaded);
    status->set_last_load_status(d->last_load);
    status->set_last_load_at(pt::to_iso_string(d->last_load_at));
    status->set_nb_threads(d->nb_threads);
    for(auto data_sources: d->meta.data_sources){
        status->add_data_sources(data_sources);
    }
    return result;
}

pbnavitia::Response Worker::metadatas() {
    pbnavitia::Response result;
    auto metadatas = result.mutable_metadatas();
    const auto d = *data;
    metadatas->set_start_production_date(bg::to_iso_string(d->meta.production_date.begin()));
    metadatas->set_end_production_date(bg::to_iso_string(d->meta.production_date.end()));
    metadatas->set_shape(d->meta.shape);
    metadatas->set_status("running");
    for(type::Contributor* contributor : d->pt_data.contributors){
        metadatas->add_contributors(contributor->uri);
    }
    return result;
}

void Worker::init_worker_data(){
    if((*this->data)->last_load_at != this->last_load_at || !planner){
        planner = std::unique_ptr<routing::RAPTOR>(new routing::RAPTOR(*(*this->data)));
        street_network_worker = std::unique_ptr<streetnetwork::StreetNetwork>(new streetnetwork::StreetNetwork((*this->data)->geo_ref));
        this->last_load_at = (*this->data)->last_load_at;

        LOG4CPLUS_INFO(logger, "instanciation du planner");
    }
}

pbnavitia::Response Worker::autocomplete(const pbnavitia::PlacesRequest & request) {
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    return navitia::autocomplete::autocomplete(request.q(),
            vector_of_pb_types(request), request.depth(), request.count(),
            vector_of_admins(request), request.search_type(), *(*this->data));
}


pbnavitia::Response Worker::next_stop_times(const pbnavitia::NextStopTimeRequest &request,
        pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    this->init_worker_data();
    try {
        switch(api){
        case pbnavitia::NEXT_DEPARTURES:
            return timetables::next_departures(request.departure_filter(),
                    request.from_datetime(), request.duration(),
                    request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *(*this->data), request.count(),
                    request.start_page());
        case pbnavitia::NEXT_ARRIVALS:
            return timetables::next_arrivals(request.arrival_filter(),
                    request.from_datetime(), request.duration(),
                    request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(),
                    *(*this->data), request.count(), request.start_page());
        case pbnavitia::STOPS_SCHEDULES:
            return timetables::stops_schedule(request.departure_filter(),
                    request.arrival_filter(), request.from_datetime(),
                    request.duration(), request.depth(), *(*this->data));
        case pbnavitia::DEPARTURE_BOARDS:
            return timetables::departure_board(request.departure_filter(),
                    request.from_datetime(), request.duration(),request.max_stop_date_times(),
                    request.interface_version(), request.count(),
                    request.start_page(), *(*this->data));
        case pbnavitia::ROUTE_SCHEDULES:
            return timetables::route_schedule(request.departure_filter(),
                    request.from_datetime(), request.duration(),
                    request.interface_version(), request.depth(),
                    request.count(), request.start_page(), *(*this->data));
        default:
            LOG4CPLUS_WARN(logger, "On a reçu une requête time table inconnue");
            pbnavitia::Response response;
            fill_pb_error(pbnavitia::Error::unknown_api, "Unknown time table api",
                    response.mutable_error());
            return response;
        }

    } catch (navitia::ptref::parsing_error error) {
        LOG4CPLUS_ERROR(logger, "Error in the ptref request  : "+ error.more);
        pbnavitia::Response response;
        const auto str_error = "Unknow filter : " + error.more;
        fill_pb_error(pbnavitia::Error::bad_filter, str_error, response.mutable_error());
        return response;
    }
}


pbnavitia::Response Worker::proximity_list(const pbnavitia::PlacesNearbyRequest &request) {
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    type::EntryPoint ep((*data)->get_type_of_id(request.uri()), request.uri());
    auto coord = this->coord_of_entry_point(ep);
    return proximitylist::find(coord, request.distance(), vector_of_pb_types(request),
                               request.depth(), request.count(), request.start_page(),
                               *(*this->data));
}


type::GeographicalCoord Worker::coord_of_entry_point(const type::EntryPoint & entry_point) {
    type::GeographicalCoord result;
    if(entry_point.type == Type_e::Address){
        auto way = (*this->data)->geo_ref.way_map.find(entry_point.uri);
        if (way != (*this->data)->geo_ref.way_map.end()){
            const auto geo_way = (*this->data)->geo_ref.ways[way->second];
            result = geo_way->nearest_coord(entry_point.house_number, (*this->data)->geo_ref.graph);
        }
    } else if (entry_point.type == Type_e::StopPoint) {
        auto sp_it = (*this->data)->pt_data.stop_points_map.find(entry_point.uri);
        if(sp_it != (*this->data)->pt_data.stop_points_map.end()) {
            result = sp_it->second->coord;
        }
    } else if (entry_point.type == Type_e::StopArea) {
           auto sa_it = (*this->data)->pt_data.stop_areas_map.find(entry_point.uri);
           if(sa_it != (*this->data)->pt_data.stop_areas_map.end()) {
               result = sa_it->second->coord;
           }
    } else if (entry_point.type == Type_e::Coord) {
        result = entry_point.coordinates;
    } else if (entry_point.type == Type_e::Admin) {
        auto it_admin = (*this->data)->geo_ref.admin_map.find(entry_point.uri);
        if (it_admin != (*this->data)->geo_ref.admin_map.end()) {
            const auto admin = (*this->data)->geo_ref.admins[it_admin->second];
            result = admin->coord;
        }

    }
    return result;
}


type::StreetNetworkParams Worker::streetnetwork_params_of_entry_point(const pbnavitia::StreetNetworkParams & request, const bool use_second){
    type::StreetNetworkParams result;
    std::string uri;

    if(use_second){
        result.mode = type::static_data::get()->modeByCaption(request.origin_mode());
        result.set_filter(request.origin_filter());
    }else{
        result.mode = type::static_data::get()->modeByCaption(request.destination_mode());
        result.set_filter(request.destination_filter());
    }
    switch(result.mode){
        case type::Mode_e::Bike:
            result.offset = (*data)->geo_ref.bike_offset;
            result.distance = request.bike_distance();
            result.speed = request.bike_speed();
            break;
        case type::Mode_e::Car:
            result.offset = (*data)->geo_ref.car_offset;
            result.distance = request.car_distance();
            result.speed = request.car_speed();
            break;
        case type::Mode_e::Vls:
            result.offset = (*data)->geo_ref.vls_offset;
            result.distance = request.vls_distance();
            result.speed = request.vls_speed();
            break;
        default:
            result.offset = 0;
            result.distance = request.walking_distance();
            result.speed = request.walking_speed();
            break;
    }
    return result;
}


pbnavitia::Response Worker::place_uri(const pbnavitia::PlaceUriRequest &request) {
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    this->init_worker_data();
    pbnavitia::Response pb_response;

    if(request.uri().size()>6 && request.uri().substr(0,6) == "coord:") {
        type::EntryPoint ep(type::Type_e::Coord, request.uri());
        auto coord = this->coord_of_entry_point(ep);
        auto tmp = proximitylist::find(coord, 100, {type::Type_e::Address}, 1, 1, 0, *(*this->data));
        if(tmp.places_nearby().size() == 1){
            auto place = pb_response.add_places();
            place->CopyFrom(tmp.places_nearby(0));
        }
        return pb_response;
    }
    pbnavitia::Place* place = pb_response.add_places();
    auto it_sa = (*data)->pt_data.stop_areas_map.find(request.uri());
    if(it_sa != (*data)->pt_data.stop_areas_map.end()) {
        fill_pb_object(it_sa->second, **data, place->mutable_stop_area(), 1);
        place->set_embedded_type(pbnavitia::STOP_AREA);
        place->set_name(place->stop_area().name());
        place->set_uri(place->stop_area().uri());
    } else {
        auto it_sp = (*data)->pt_data.stop_points_map.find(request.uri());
        if(it_sp != (*data)->pt_data.stop_points_map.end()) {
            fill_pb_object(it_sp->second, **data, place->mutable_stop_point(), 1);
            place->set_embedded_type(pbnavitia::STOP_POINT);
            place->set_name(place->stop_point().name());
            place->set_uri(place->stop_point().uri());
        } else {
            auto it_poi = (*data)->geo_ref.poi_map.find(request.uri());
            if(it_poi != (*data)->geo_ref.poi_map.end()) {
                fill_pb_object((*data)->geo_ref.pois[it_poi->second], **data,
                        place->mutable_poi(), 1);
                place->set_embedded_type(pbnavitia::POI);
                place->set_name(place->poi().name());
                place->set_uri(place->poi().uri());
            } else {
                auto it_admin = (*data)->geo_ref.admin_map.find(request.uri());
                if(it_admin != (*data)->geo_ref.admin_map.end()) {
                    fill_pb_object((*data)->geo_ref.admins[it_admin->second],
                            **data, place->mutable_administrative_region(), 1);
                    place->set_embedded_type(pbnavitia::ADMINISTRATIVE_REGION);
                    place->set_name(place->administrative_region().name());
                    place->set_uri(place->administrative_region().uri());
                } else {
                    pb_response.clear_places();
                }
            }
        }
    }
    return pb_response;
}

pbnavitia::Response Worker::journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api) {
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    this->init_worker_data();

    Type_e origin_type = (*data)->get_type_of_id(request.origin());
    type::EntryPoint origin = type::EntryPoint(origin_type, request.origin());

    if (origin.type == type::Type_e::Address || origin.type == type::Type_e::Admin) {
        origin.coordinates = this->coord_of_entry_point(origin);
    }

    type::EntryPoint destination;
    if(api != pbnavitia::ISOCHRONE) {
        Type_e destination_type = (*data)->get_type_of_id(request.destination());
        destination = type::EntryPoint(destination_type, request.destination());
        if (destination.type == type::Type_e::Address || destination.type == type::Type_e::Admin) {
            destination.coordinates = this->coord_of_entry_point(destination);
        }
    }

    std::vector<std::string> forbidden;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden.push_back(request.forbidden_uris(i));

    std::vector<std::string> datetimes;
    for(int i = 0; i < request.datetimes_size(); ++i)
        datetimes.push_back(request.datetimes(i));

    /// Récupération des paramètres de rabattement au départ
    if ((origin.type == type::Type_e::Address) || (origin.type == type::Type_e::Coord)
            || (origin.type == type::Type_e::Admin)){
        origin.streetnetwork_params = this->streetnetwork_params_of_entry_point(request.streetnetwork_params());
    }
    /// Récupération des paramètres de rabattement à l'arrivée
    if ((destination.type == type::Type_e::Address) || (destination.type == type::Type_e::Coord)
            || (destination.type == type::Type_e::Admin)){
        destination.streetnetwork_params = this->streetnetwork_params_of_entry_point(request.streetnetwork_params(), false);
    }
/// Accessibilité, il faut initialiser ce paramètre
    type::AccessibiliteParams accessibilite_params;

    if(api != pbnavitia::ISOCHRONE){
        return routing::make_response(*planner, origin, destination, datetimes,
                request.clockwise(), request.streetnetwork_params().walking_speed(),
                request.streetnetwork_params().walking_distance(), accessibilite_params,
                forbidden, *street_network_worker, request.max_duration(),
                request.max_transfers());
    } else {
        return navitia::routing::make_isochrone(*planner, origin, request.datetimes(0),
                request.clockwise(), request.streetnetwork_params().walking_speed(),
                request.streetnetwork_params().walking_distance(), accessibilite_params,
                forbidden, *street_network_worker, request.max_duration(),
                request.max_transfers());
    }
}


pbnavitia::Response Worker::pt_ref(const pbnavitia::PTRefRequest &request){
    boost::shared_lock<boost::shared_mutex> lock((*data)->load_mutex);
    return navitia::ptref::query_pb(get_type(request.requested_type()),
                                    request.filter(), request.depth(),
                                    request.start_page(), request.count(),
                                    *(*this->data));
}


pbnavitia::Response Worker::dispatch(const pbnavitia::Request& request) {
    pbnavitia::Response result ;
    if (! (*data)->loaded){
        fill_pb_error(pbnavitia::Error::service_unavailable, "The service is loading data", result.mutable_error());
        return result;
    }
    switch(request.requested_api()){
        case pbnavitia::STATUS: return status(); break;
        case pbnavitia::places: return autocomplete(request.places()); break;
        case pbnavitia::place_uri: return place_uri(request.place_uri()); break;
        case pbnavitia::ROUTE_SCHEDULES:
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::STOPS_SCHEDULES:
        case pbnavitia::DEPARTURE_BOARDS:
            return next_stop_times(request.next_stop_times(), request.requested_api()); break;
        case pbnavitia::ISOCHRONE:
        case pbnavitia::PLANNER: return journeys(request.journeys(), request.requested_api()); break;
        case pbnavitia::places_nearby: return proximity_list(request.places_nearby()); break;
        case pbnavitia::PTREFERENTIAL: return pt_ref(request.ptref()); break;
        case pbnavitia::METADATAS : return metadatas(); break;
        default:
            LOG4CPLUS_WARN(logger, "Unknown API : " + API_Name(request.requested_api()));
            fill_pb_error(pbnavitia::Error::unknown_api, "Unknown API", result.mutable_error());
            break;
    }

    return result;
}

}
