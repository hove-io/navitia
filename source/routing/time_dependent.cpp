#include "time_dependent.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/foreach.hpp>

namespace navitia { namespace routing { namespace timedependent {

struct found_goal{};

class goal_visitor : public boost::default_dijkstra_visitor
{
public:
  goal_visitor(vertex_t goal) : m_goal(goal) {}

  void examine_vertex(vertex_t u, const Graph&) const {
    if(u == m_goal)
      throw found_goal();
  }
private:
  vertex_t m_goal;
};


TimeDependent::TimeDependent(const type::Data & global_data) : data(global_data.pt_data),
       // On crée un nœud par route point, deux par stopArea, deux par stopPoint
       graph(data.route_points.size() + data.stop_areas.size() + data.stop_points.size()),
       stop_point_offset(data.stop_areas.size()),
       route_point_offset(stop_point_offset + data.stop_points.size()),
       preds(boost::num_vertices(graph)),
       distance(boost::num_vertices(graph))
   {
    this->build_graph();
   }


void  TimeDependent::build_graph(){
    // 1. On rajoute un arc pour chaque segment de route
    BOOST_FOREACH(type::Route route, data.routes){
        for(size_t i = 1; i < route.route_point_list.size(); ++i){
            boost::add_edge(route.route_point_list[i-1] + route_point_offset, route.route_point_list[i]+ route_point_offset, graph);
        }
    }

    // 2. On relie les stoppoints aux stop_areas
    BOOST_FOREACH(type::StopPoint stop_point, data.stop_points){
        boost::add_edge(stop_point.stop_area_idx, stop_point.idx + stop_point_offset, Edge(60), graph);
        boost::add_edge(stop_point.idx + stop_point_offset, stop_point.stop_area_idx, Edge(60), graph);
    }


    // 3. On rajoute un arc entre route_points et stop_points de durée constante
    BOOST_FOREACH(type::RoutePoint route_point, data.route_points){
        // Deux minutes de battement à l'arrivée
        boost::add_edge(route_point.stop_point_idx + stop_point_offset, route_point.idx + route_point_offset, Edge(120), graph);
        // 0 à l'arrivée
        boost::add_edge(route_point.idx + route_point_offset, route_point.stop_point_idx + stop_point_offset, Edge(0), graph);
    }

    // 4. On rajoute les horaires
    BOOST_FOREACH(type::VehicleJourney vj, data.vehicle_journeys){
        for(size_t i = 1; i < vj.stop_time_list.size(); ++i){
            type::StopTime stop1 = data.stop_times[vj.stop_time_list[i-1]];
            type::StopTime stop2 = data.stop_times[vj.stop_time_list[i]];
            ValidityPatternTime vpt1(vj.validity_pattern_idx, stop1.departure_time);
            ValidityPatternTime vpt2(vj.validity_pattern_idx, stop2.arrival_time);
            if(stop1.departure_time > stop2.arrival_time){
                std::cout << "Incohérence " << vj.external_code << std::endl;
                std::cout << stop1.departure_time << " " << stop2.arrival_time << std::endl;
            }
            bool b;
            edge_t e;
            boost::tie(e, b) = boost::edge(stop1.route_point_idx + route_point_offset, stop2.route_point_idx + route_point_offset, graph);
            graph[e].t.add(vpt1, vpt2, vj.idx);
        }
    }

    // 4.bis on rajoute les correspondances
    BOOST_FOREACH(type::Connection conn, data.connections){
        if(data.stop_points[conn.departure_stop_point_idx].stop_area_idx != data.stop_points[conn.destination_stop_point_idx].stop_area_idx){
            bool b;
            edge_t e;
            boost::tie(e, b) = boost::add_edge(conn.departure_stop_point_idx + stop_point_offset, conn.destination_stop_point_idx + stop_point_offset, graph);
            graph[e].t.constant_duration = conn.duration;
            boost::tie(e, b) = boost::add_edge(conn.destination_stop_point_idx + stop_point_offset, conn.departure_stop_point_idx + stop_point_offset, graph);
            graph[e].t.constant_duration = conn.duration;
        }
    }

    // 5. On trie les horaires de chaque arc et on garde le temps le plus court pour l'heuristique A*
    BOOST_FOREACH(edge_t e, boost::edges(graph)){
        std::sort(graph[e].t.time_table.begin(), graph[e].t.time_table.end());
    }
}


DateTime TimeTable::eval(const DateTime & departure, const type::PT_Data &data) const{
    if(departure == DateTime::inf)
        return departure;

    // Si on a des durée constantes, c'est simple à trouver
    if(this->constant_duration >= 0)
        return departure + constant_duration;

    // On cherche le prochain départ le jour même par dichotomie
    auto it = std::lower_bound(time_table.begin(), time_table.end(), departure.hour(),
                               [](const TimeTableElement & a, int hour){return a.first.hour < hour;});
    auto end = time_table.end();
    for(; it != end; ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(vp.check(departure.date())){
            return DateTime(departure.date(), it->second.hour);
        }
    }

    // Zut ! on a rien trouvé le jour même, on regarde le lendemain au plus tôt
    for(auto it = this->time_table.begin(); it != end; ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(vp.check(departure.date() + 1)){
            return DateTime(departure.date() + 1, it->second.hour);
        }
    }

    // Bon, on en fait cet arc n'est jamais utilisable
    return DateTime::inf;
}

std::pair<DateTime, type::idx_t> TimeTable::first_departure(DateTime departure, const type::PT_Data &data) const{
    if(departure == DateTime::inf)
        return std::make_pair(departure, type::invalid_idx);

    if(this->constant_duration >= 0)
        return std::make_pair(departure, type::invalid_idx);


    // On cherche le prochain départ le jour même par dichotomie
    auto it = std::lower_bound(time_table.begin(), time_table.end(), departure.hour(),
                               [](const TimeTableElement & a, int hour){return a.first.hour < hour;});
    auto end = time_table.end();
    for(; it != end; ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(vp.check(departure.date())){
            return std::make_pair(DateTime(departure.date(), it->first.hour), it->vj);
        }
    }

    // Zut ! on a rien trouvé le jour même, on regarde le lendemain au plus tôt
    for(auto it = this->time_table.begin(); it != end; ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(vp.check(departure.date() + 1)){
            return std::make_pair(DateTime(departure.date() + 1, it->first.hour), it->vj);
        }
    }

    // Bon, on en fait cet arc n'est jamais utilisable
    return std::make_pair(DateTime::inf, type::invalid_idx);
}


struct Combine{
    const type::PT_Data & data;
    Combine(const type::PT_Data & data) : data(data) {}
    DateTime operator()(DateTime dt, const TimeTable & t) const {
        return t.eval(dt, data);
    }
    DateTime operator()(DateTime dt, const DateTime & t) const {
        dt.increment(t.hour());
        return dt;
    }
};

struct edge_less{
    bool operator()(DateTime a, DateTime b) const {
        return a < b;
    }
    bool operator ()(const TimeTable&, DateTime) const{return false;}
};


Path TimeDependent::compute(type::idx_t dep, type::idx_t arr, int hour, int day, senscompute /*sens*/){
    DateTime start_time(day, hour);

    try{
        boost::dijkstra_shortest_paths(this->graph, dep,
                                       boost::distance_map(&distance[0])
                                       .predecessor_map(&preds[0])
                                       .weight_map(boost::get(&Edge::t, graph))
                                       .distance_combine(Combine(this->data))
                                       .distance_inf(DateTime::infinity())
                                       .distance_zero(start_time)
                                       .distance_compare(edge_less())
                                       .visitor(goal_visitor(arr))
                                       );
    } catch(found_goal){}

    return makePath(arr);
}


Path TimeDependent::makePath(type::idx_t arr) {
    Path result;

    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(preds[v] != v)
            count++;
    }
    result.percent_visited = 100 * count/boost::num_vertices(this->graph);

    std::vector<vertex_t> path;
    vertex_t arrival = arr;
    while(preds[arrival] != arrival){
        path.push_back(arrival);
        arrival = preds[arrival];
    }
    std::reverse(path.begin(), path.end());

    PathItem item;
    bool first = true;
    for(size_t i = 1; i < path.size(); ++i){
        vertex_t u = path[i-1];
        vertex_t v = path[i];

        // On commence un segment TC
        if(!is_route_point(u) && is_route_point(v)){
            first = true;
            item = PathItem();
            item.type = public_transport;
            item.stop_points.push_back(data.route_points[v - route_point_offset].stop_point_idx);
        }

        // On finit un segment TC
        else if(is_route_point(u) && !is_route_point(v)){
            item.arrival = distance[u];
            result.items.push_back(item);
        }

        // On a une correspondance
        else if(this->is_stop_point(u) && this->is_stop_point(v)) {
            const type::StopPoint & source = data.stop_points[u - stop_point_offset];
            const type::StopPoint & target = data.stop_points[v - stop_point_offset];

            item = PathItem();
            item.departure = distance[u];
            item.arrival = distance[v];
            item.type = walking;
            item.stop_points.push_back(source.idx);
            item.stop_points.push_back(target.idx);
            result.items.push_back(item);
        }

        // On est au milieu d'un segment TC
        if(is_route_point(u) && is_route_point(v)){
            if(first){
                edge_t e = boost::edge(u, v, this->graph).first;
                boost::tie(item.departure, item.vj_idx) = graph[e].t.first_departure(distance[u], this->data);
                first = false;
            }
            item.stop_points.push_back(data.route_points[v - route_point_offset].stop_point_idx);
        }
    }


    if(result.items.size() > 0)
        result.duration = result.items.back().arrival - result.items.front().departure;
    else
        result.duration = 0;
    return result;
}

}}}
