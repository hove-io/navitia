#include "static_raptor.h"
#include <unordered_map>

#include "utils/timer.h"

namespace navitia { namespace routing {

void StaticRaptor::compute_route_durations(){
    Timer t("Construction des routes and co");
    int total = 0;
    int null = 0;
    std::vector< std::vector<uint32_t> > route_durations(data.routes.size());

    for(const type::Route & route : data.routes){
        if(route.vehicle_journey_list.size() > 0){
            const type::VehicleJourney & first = data.vehicle_journeys[route.vehicle_journey_list.front()];
            std::vector<uint32_t> durations(first.stop_time_list.size(), std::numeric_limits<int>::max());
            durations[0] = 0;
            for(type::idx_t vj_idx : route.vehicle_journey_list){
                const type::VehicleJourney & vj = data.vehicle_journeys[vj_idx];
                int departure = data.stop_times[vj.stop_time_list[0]].departure_time;
                for(size_t i = 1; i < first.stop_time_list.size(); ++i){
                    const type::StopTime &current = data.stop_times[vj.stop_time_list[i]];
                    durations[i] = std::min(durations[i], static_cast<uint32_t>(current.arrival_time - departure));
                    total++;
                    if(durations[i] == 0) null++;
                }
            }
            route_durations[route.idx] = std::move(durations);
        }
    }

    for(const type::StopPoint & stop_point : data.stop_points){
        std::unordered_map<idx_t, uint32_t> duration;

        // Pour chaque route passant par le stop point
        for(idx_t route_point_idx : stop_point.route_point_list) {
            const type::RoutePoint & route_point = data.route_points[route_point_idx];
            const type::Route & route = data.routes[route_point.route_idx];
            auto & route_duration = route_durations[route.idx];

            uint32_t zero = route_duration[route_point.order];
            // Pour chaque route point après sur la route
            for(size_t i = route_point.order + 1; i < route_duration.size(); ++i){
                idx_t dest_sp_idx = data.route_points[route.route_point_list[i]].stop_point_idx;
                if(duration.find(dest_sp_idx) == duration.end())
                    duration[dest_sp_idx] = route_duration[i] - zero;
                else
                    duration[dest_sp_idx] = std::min(route_duration[i] - zero, duration[dest_sp_idx]);
            }
        }
        out_edges[stop_point.idx].reserve(duration.size() + this->data.stop_point_connections.size());
        for(auto pair : duration){
            out_stop_point o;
            o.stop_point_idx = pair.first;
            o.duration = pair.second;
            out_edges[stop_point.idx].push_back(o);
        }
        for(const type::Connection & conn : data.stop_point_connections[stop_point.idx]){
            out_stop_point o;
            o.stop_point_idx = conn.destination_stop_point_idx;
            o.duration = conn.duration;
            out_edges[stop_point.idx].push_back(o);
        }
    }
}



void StaticRaptor::init(idx_t departure_idx){
    this->marked_stop.reset();
    best_durations.assign(this->data.stop_points.size(), std::numeric_limits<uint32_t>::max());
    heap.clear();
    for(idx_t stop_point_idx : this->data.stop_areas[departure_idx].stop_point_list){
        this->marked_stop.set(stop_point_idx);
        best_durations[stop_point_idx] = 0;
        handles[stop_point_idx] = heap.push(stop_point_idx);
    }
}

void StaticRaptor::mark(idx_t stop_point_idx){
    if(!marked_stop[stop_point_idx]){
        handles[stop_point_idx] = heap.push(stop_point_idx);
        marked_stop.set(stop_point_idx);
    }
    else{
        heap.increase(handles[stop_point_idx]);
    }
}

Path StaticRaptor::compute(idx_t departure_idx, idx_t arrival_idx, int, int, senscompute){

    Path result;
    init(departure_idx);
    std::vector<idx_t> arrivals;
    for(auto arrival : data.stop_areas[arrival_idx].stop_point_list)
        arrivals.push_back(arrival);


    while(!heap.empty()){
        idx_t current = heap.top();
        heap.pop();
        uint32_t zero = best_durations[current];

        if(std::find(arrivals.begin(), arrivals.end(), current) != arrivals.end()){
            return result;
        }

        for(out_stop_point edge : out_edges[current]){
            uint32_t new_duration = zero + edge.duration;
            uint32_t & old_duration = best_durations[edge.stop_point_idx];
            if(new_duration < old_duration){
                old_duration = new_duration;
                mark(edge.stop_point_idx);
            }
        }
    }

    return result;
}

}}
