#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"
namespace navitia { namespace routing { namespace raptor {

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_string(date_time);
}

pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);

    pbnavitia::Planner * planner = pb_response.mutable_planner();
    planner->set_response_type(pbnavitia::ITINERARY_FOUND);
    for(Path path : paths) {
        pbnavitia::Journey * pb_journey = planner->add_journey();
        pb_journey->set_duration(path.duration);
        pb_journey->set_nb_transfers(path.nb_changes);
        pb_journey->set_requested_date_time(boost::posix_time::to_iso_string(path.request_time));
        for(PathItem & item : path.items){
            pbnavitia::Section * pb_section = pb_journey->add_section();
            pb_section->set_arrival_date_time(iso_string(d, item.arrival.date(), item.arrival.hour()));
            pb_section->set_departure_date_time(iso_string(d, item.departure.date(), item.departure.hour()));
            if(item.type == public_transport)
                pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
            else
                pb_section->set_type(pbnavitia::TRANSFER);
            if(item.type == public_transport && item.vj_idx != type::invalid_idx){
                const type::VehicleJourney & vj = d.pt_data.vehicle_journeys[item.vj_idx];
                const type::Route & route = d.pt_data.routes[vj.route_idx];
                const type::Line & line = d.pt_data.lines[route.line_idx];
                fill_pb_object(line.idx, d, pb_section->mutable_line());
            }
            for(navitia::type::idx_t stop_point : item.stop_points){
                fill_pb_object(stop_point, d, pb_section->add_stop_point());
            }
            if(item.stop_points.size() >= 2) {
                pbnavitia::PlaceMark * origin_place_mark = pb_section->mutable_origin();
                origin_place_mark->set_type(pbnavitia::STOPAREA);
                fill_pb_object(d.pt_data.stop_points[item.stop_points.front()].stop_area_idx, d, origin_place_mark->mutable_stop_area());
                pbnavitia::PlaceMark * destination_place_mark = pb_section->mutable_destination();
                destination_place_mark->set_type(pbnavitia::STOPAREA);
                fill_pb_object(d.pt_data.stop_points[item.stop_points.back()].stop_area_idx, d, destination_place_mark->mutable_stop_area());
            }

        }
    }

    return pb_response;
}

std::vector<std::pair<type::idx_t, double> > get_stop_points(const type::EntryPoint &ep, const type::Data & data, georef::StreetNetworkWorker & worker){
    std::vector<std::pair<type::idx_t, double> > result;

    switch(ep.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it = data.pt_data.stop_area_map.find(ep.external_code);
        if(it!= data.pt_data.stop_area_map.end()) {
            for(auto spidx : data.pt_data.stop_areas[it->second].stop_point_list) {
                result.push_back(std::make_pair(spidx, 0));
            }
        }
    } break;
    case type::Type_e::eStopPoint: {
        auto it = data.pt_data.stop_point_map.find(ep.external_code);
        if(it != data.pt_data.stop_point_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second].idx, 0));
        }
    } break;
    case type::Type_e::eCoord: {
        result = worker.find_nearest(ep.coordinates, data.pt_data.stop_point_proximity_list, 300);
    } break;
    default: break;
    }
    return result;
}

vector_idxretour to_idxretour(std::vector<std::pair<type::idx_t, double> > elements, int hour, int day){
    vector_idxretour result;
    for(auto item : elements) {
        int temps = hour + (item.second / 80);
        int jour;
        if(temps > 86400) {
            temps = temps % 86400;
            jour = day + 1;
        } else {
            jour = day;
        }
        result.push_back(std::make_pair(item.first, type_retour(navitia::type::invalid_idx, DateTime(jour, temps), 0, (item.second / 80))));
    }
    return result;
}


pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &origin, const type::EntryPoint &destination,
                                  const boost::posix_time::ptime &datetime, bool clockwise,
                                  georef::StreetNetworkWorker & worker) {
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::PLANNER);

    if(!raptor.data.meta.production_date.contains(datetime.date())) {
        response.mutable_planner()->set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
        return response;
    }

    auto departures = get_stop_points(origin, raptor.data, worker);
    auto destinations = get_stop_points(destination, raptor.data, worker);
    if(departures.size() == 0 && destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
    }

    if(departures.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    if(destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_DESTINATION_POINT);
        return response;
    }

    std::vector<Path> result;

    int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
    int time = datetime.time_of_day().total_seconds();

    if(clockwise)
        result = raptor.compute_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day));
    else
        result = raptor.compute_reverse_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day));

    if(result.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_SOLUTION);
        return response;
    }

    for(Path & path : result)
        path.request_time = datetime;
    return make_pathes(result, raptor.data);
}


pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &origin, const type::EntryPoint &destination,
                                  const std::vector<std::string> &datetimes_str, bool clockwise,
                                  georef::StreetNetworkWorker & worker) {
    pbnavitia::Response response;
    response.set_requested_api(pbnavitia::PLANNER);

    std::vector<boost::posix_time::ptime> datetimes;
    for(std::string datetime: datetimes_str){
        try {
            boost::posix_time::ptime ptime;
            ptime = boost::posix_time::from_iso_string(datetime);
            if(!raptor.data.meta.production_date.contains(ptime.date())) {
                response.mutable_planner()->set_response_type(pbnavitia::DATE_OUT_OF_BOUNDS);
                response.set_info("Example of invalid date: " + datetime);
                return response;
            }
            datetimes.push_back(ptime);
        } catch(...){
            response.set_error("Impossible to parse date " + datetime);
            return response;
        }
    }
    if(clockwise)
        std::sort(datetimes.begin(), datetimes.end(), 
                  [](boost::posix_time::ptime dt1, boost::posix_time::ptime dt2){return dt1 > dt2;});
    else
        std::sort(datetimes.begin(), datetimes.end(), 
                  [](boost::posix_time::ptime dt1, boost::posix_time::ptime dt2){return dt1 < dt2;});

    auto departures = get_stop_points(origin, raptor.data, worker);
    auto destinations = get_stop_points(destination, raptor.data, worker);
    if(departures.size() == 0 && destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
    }

    if(departures.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_ORIGIN_POINT);
        return response;
    }

    if(destinations.size() == 0){
        response.mutable_planner()->set_response_type(pbnavitia::NO_DESTINATION_POINT);
        return response;
    }

    std::vector<Path> result;

    DateTime borne;
    if(!clockwise)
        borne = DateTime::min;

    for(boost::posix_time::ptime datetime : datetimes){
        std::vector<Path> tmp;
        int day = (datetime.date() - raptor.data.meta.production_date.begin()).days();
        int time = datetime.time_of_day().total_seconds();

        if(clockwise)
            tmp = raptor.compute_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day), borne);
        else
            tmp = raptor.compute_reverse_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day), borne);
        if(tmp.size() > 0) {
            tmp.back().request_time = datetime;
            result.push_back(tmp.back());
            borne = tmp.back().items.back().arrival;
        }
        else
            result.push_back(Path());
    }
    if(clockwise)
        std::reverse(result.begin(), result.end());

    return make_pathes(result, raptor.data);
}

}}}
