#include "time_dependent.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/foreach.hpp>
namespace navitia { namespace routing {


DateTime operator+(DateTime dt, int seconds) {
    dt.hour += seconds;
    dt.normalize();
    return dt;
}

TimeDependent::TimeDependent(const type::PT_Data & data) : data(data),
       // On crée un nœud par route point, deux par stopArea, deux par stopPoint
       graph(data.route_points.size() + 2 * data.stop_areas.size() + 2* data.stop_points.size()),
       stop_area_offset_dep(data.route_points.size()),
       stop_area_offset_arr(stop_area_offset_dep + data.stop_areas.size()),
       stop_point_offset_dep(stop_area_offset_arr + data.stop_areas.size()),
       stop_point_offset_arr(stop_point_offset_dep + data.stop_points.size())
   {}

void  TimeDependent::build_graph(){
    // 1. On rajoute un arc pour chaque segment de route
    BOOST_FOREACH(type::Route route, data.routes){
        for(size_t i = 1; i < route.route_point_list.size(); ++i){
            boost::add_edge(route.route_point_list[i-1], route.route_point_list[i], graph);
        }
    }

    BOOST_FOREACH(type::StopPoint stop_point, data.stop_points){
        // 2. On ajoute les correspondances entre routepoints d'un même stopPoint
        BOOST_FOREACH(type::idx_t rp_idx1, stop_point.route_point_list){
            BOOST_FOREACH(type::idx_t rp_idx2, stop_point.route_point_list){
                if(rp_idx1 != rp_idx2){
                    // 120 secondes pour une correspondance intra-stoppoint
                    boost::add_edge(rp_idx1, rp_idx2, Edge(120), graph);
                }
            }
        }

        // 3. On relie les stoppoints aux stop_areas
        boost::add_edge(stop_point.stop_area_idx + stop_area_offset_dep, stop_point.idx + stop_point_offset_dep, Edge(60), graph);
        boost::add_edge(stop_point.idx + stop_point_offset_arr, stop_point.stop_area_idx + stop_area_offset_arr, Edge(60), graph);
    }


    // 3. On rajoute un arc entre route_points et stop_points de durée constante
    BOOST_FOREACH(type::RoutePoint route_point, data.route_points){
        // Deux minutes de battement à l'arrivée
        boost::add_edge(route_point.stop_point_idx + stop_point_offset_dep, route_point.idx, Edge(120), graph);
        // 0 à l'arrivée
        boost::add_edge(route_point.idx, route_point.stop_point_idx + stop_point_offset_arr, Edge(0), graph);
    }

    // 4. On rajoute les horaires
    BOOST_FOREACH(type::VehicleJourney vj, data.vehicle_journeys){
        for(size_t i = 1; i < vj.stop_time_list.size(); ++i){
            type::StopTime stop1 = data.stop_times[vj.stop_time_list[vj.stop_time_list[i-1]]];
            type::StopTime stop2 = data.stop_times[vj.stop_time_list[vj.stop_time_list[i]]];
            ValidityPatternTime vpt1(vj.validity_pattern_idx, stop1.departure_time);
            ValidityPatternTime vpt2(vj.validity_pattern_idx, stop2.departure_time);
            bool b;
            edge_t e;
            boost::tie(e, b) = boost::edge(stop1.route_point_idx, stop2.route_point_idx, graph);
            BOOST_ASSERT(b);
            graph[e].t.add(vpt1, vpt2);
        }
    }

    // 5. On trie les horaires de chaque arc
    BOOST_FOREACH(edge_t e, boost::edges(graph)){
        std::sort(graph[e].t.time_table.begin(), graph[e].t.time_table.end());
    }
}

DateTime TimeTable::eval(DateTime departure, const type::PT_Data &data) const{

    if(this->constant_duration >= 0)
        return departure + constant_duration;

    DateTime next_day;
    // Todo : optimiser par dichotomie car les time_tables sont triées par heure de départ
    BOOST_FOREACH(auto pair, this->time_table){
        ValidityPatternTime dep = pair.first;
        type::ValidityPattern vp = data.validity_patterns[dep.vp_idx];
        // On garde la première date du lendemain si jamais on ne trouve rien
        if(next_day == DateTime::infinity() && vp.check(departure.date + 1)){
            next_day.date = departure.date + 1;
            next_day.hour = pair.second.hour;
        }

        // Aha ! on a trouvé le premier départ le bon jour !
        if(dep.hour >= departure.hour && vp.check(departure.date)){
            DateTime result;
            result.hour = pair.second.hour;
            result.date = departure.date;
            result.normalize();
            return result;
        }
    }
    return next_day;
}

struct Combine{
    const type::PT_Data & data;
    Combine(const type::PT_Data & data) : data(data) {}
    DateTime operator()(DateTime dt, const TimeTable & t) const {
        return t.eval(dt, data);
    }
};

struct edge_less{
    bool operator()(DateTime a, DateTime b) const {
        return a < b;
    }
    bool operator ()(const TimeTable&, DateTime) const{return false;}
};

std::vector<std::string> TimeDependent::compute(const type::StopArea &departure, const type::StopArea &arrival){
    std::vector<vertex_t> preds(boost::num_vertices(this->graph));
    std::vector<DateTime> distance(boost::num_vertices(this->graph));

    DateTime start_time;
    start_time.date = 1;
    start_time.hour = 8*3600;

    boost::typed_identity_property_map<edge_t> identity;
    boost::dijkstra_shortest_paths(this->graph, departure.idx + stop_area_offset_dep,
                                   boost::distance_map(&distance[0])
            .predecessor_map(&preds[0])
            .weight_map(boost::get(&Edge::t, graph))
            .distance_combine(Combine(this->data))
            .distance_inf(DateTime::infinity())
            .distance_zero(start_time)
            .distance_compare(edge_less())
            );

    std::vector<std::string> result;
    return result;
}


}}
