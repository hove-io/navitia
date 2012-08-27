#include "time_dependent.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "utils/timer.h"
namespace navitia { namespace routing { namespace timedependent {

class distance_heuristic : public boost::astar_heuristic<Graph, DateTime>
{
public:
  distance_heuristic(const TimeDependent & td) : td(td){}

  // Estimation de l'heure d'arrivée. Cette heure doit est sous-estimée
  DateTime operator()(Vertex u){
      DateTime result;
      result.hour = td.astar_graph.min_time.at(u);
      result.date = -1;
      return result;
  }
private:
  const TimeDependent & td;
};


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


TimeDependent::TimeDependent(const type::PT_Data & data) : data(data),
       // On crée un nœud par route point, deux par stopArea, deux par stopPoint
       graph(data.route_points.size() + data.stop_areas.size() + data.stop_points.size()),
       astar_graph(data),
       stop_point_offset(data.stop_areas.size()),
       route_point_offset(stop_point_offset + data.stop_points.size()),
       preds(boost::num_vertices(graph)),
       distance(boost::num_vertices(graph)),
       astar_dist(boost::num_vertices(graph))
   {
    BOOST_ASSERT(boost::num_vertices(this->graph) == route_point_offset + data.route_points.size());
    std::cout <<  3*data.route_points.size() - data.routes.size() + 2*data.stop_points.size() << std::endl;
    //BOOST_ASSERT(boost::num_edges(this->graph) == 3*data.route_points.size() - data.vehicle_journeys.size() + 2*data.stop_points.size() );
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
            BOOST_ASSERT(b);
            graph[e].t.add(vpt1, vpt2);
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
        graph[e].min_duration = graph[e].t.min_duration();
    }


}
void TimeDependent::build_heuristic(uint destination) {
    astar_graph.build_heuristic(destination);
}

DateTime TimeTable::eval(const DateTime & departure, const type::PT_Data &data) const{
    if(departure == DateTime::inf)
        return departure;

    // Si on a des durée constantes, c'est simple à trouver
    if(this->constant_duration >= 0)
        return departure + constant_duration;

    // On cherche le prochain départ le jour même par dichotomie
    auto it = std::lower_bound(time_table.begin(), time_table.end(), departure.hour,
                               [](const std::pair<ValidityPatternTime, ValidityPatternTime> & a, int hour){return a.first.hour < hour;});
    for(; it != time_table.end(); ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(it->first.hour >= departure.hour && vp.check(departure.date)){
           return DateTime(departure.date, it->second.hour);
        }
    }

    // Zut ! on a rien trouvé le jour même, on regarde le lendemain au plus tôt
    for(auto it = this->time_table.begin(); it != time_table.end(); ++it){
        const type::ValidityPattern & vp = data.validity_patterns[it->first.vp_idx];
        if(vp.check(departure.date + 1)){
            return DateTime(departure.date + 1, it->second.hour);
        }
    }

    // Bon, on en fait cet arc n'est jamais utilisable
    return DateTime::inf;
}

DateTime TimeTable::first_departure(DateTime departure, const type::PT_Data &data) const{
    if(departure == DateTime::infinity())
        return departure;

    if(this->constant_duration >= 0)
        return departure;

    return eval(departure, data);
}

int TimeTable::min_duration() const {
    if(constant_duration >= 0)
        return constant_duration;

    int result = std::numeric_limits<int>::max();
    BOOST_FOREACH(auto pair, this->time_table){
        if(pair.second.hour < pair.first.hour)
            std::cout << pair.first.hour << " " << pair.first.vp_idx << " → " << pair.second.hour << " " << pair.second.vp_idx << std::endl;
        if(pair.second.hour - pair.first.hour < result)
            result = pair.second.hour - pair.first.hour;
    }
    BOOST_ASSERT(result >= 0);
    return result;
}

struct Combine{
    const type::PT_Data & data;
    Combine(const type::PT_Data & data) : data(data) {}
    DateTime operator()(DateTime dt, const TimeTable & t) const {
        return t.eval(dt, data);
    }
    DateTime operator()(DateTime dt, const DateTime & t) const {
        BOOST_ASSERT(t.date == -1);
        dt.hour += t.hour;
        return dt;
    }
};

struct edge_less{
    bool operator()(DateTime a, DateTime b) const {
        return a < b;
    }
    bool operator ()(const TimeTable&, DateTime) const{return false;}
};

bool operator==(const PathItem & a, const PathItem & b) {
    return a.said == b.said && a.arrival == b.arrival && a.departure == b.departure;
}

std::ostream & operator<<(std::ostream & os, const PathItem & b){
    os << b.said << " " << b.arrival;
    return os;
}

Path TimeDependent::compute(type::idx_t dep, type::idx_t arr, int hour, int day){
    DateTime start_time;
    start_time.date = day;
    start_time.hour = hour;

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


Path TimeDependent::compute_astar(type::idx_t dep, type::idx_t arr, int hour, int day){
    DateTime start_time;
    start_time.date = day;
    start_time.hour = hour;

//    astar_graph.build_heuristic(arrival);

    try{
        boost::astar_search(this->graph, dep, distance_heuristic(*this),
                            boost::distance_map(&distance[0])
                            .predecessor_map(&preds[0])
                            .weight_map(boost::get(&Edge::t, graph))
                            .distance_combine(Combine(this->data))
                            .distance_inf(DateTime::infinity())
                            .distance_zero(start_time)
                            .distance_compare(edge_less())
                            .visitor(navitia::routing::astar::astar_goal_visitor(arr))
                            .rank_map(&astar_dist[0])
            );
    } catch(navitia::routing::astar::found_goal){}

    return makePath(arr);
}

bool TimeDependent::is_stop_area(vertex_t vertex) const {
    return vertex < stop_point_offset;
}

bool TimeDependent::is_stop_point(vertex_t vertex) const {
    return vertex >= stop_point_offset && vertex < route_point_offset;
}

bool TimeDependent::is_route_point(vertex_t vertex) const {
    return vertex >= route_point_offset;
}

Path TimeDependent::makePath(type::idx_t arr) {
    Path result;


    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(preds[v] != v)
            count++;
    }

    result.percent_visited = 100 * count/boost::num_vertices(this->graph);


    vertex_t arrival = arr;
    int precsaid = -1;
    while(preds[arrival] != arrival){
        if(is_route_point(arrival)){
            const type::StopPoint & sp = data.stop_points[data.route_points[arrival - route_point_offset].stop_point_idx];

            DateTime arrival_date = distance[arrival];
            DateTime departure_date;
            if(is_route_point(preds[arrival])){
                edge_t e = boost::edge(preds[arrival], arrival, this->graph).first;
                DateTime dt = distance[preds[arrival]];
                departure_date = graph[e].t.first_departure(dt, this->data);
            }
            PathItem item(sp.stop_area_idx, arrival_date, departure_date,
                          data.routes.at(data.route_points.at(arrival - route_point_offset).route_idx).line_idx,
                          data.route_points.at(arrival - route_point_offset).route_idx);

            result.items.push_back(item);
            if(precsaid == sp.stop_area_idx)
                ++result.nb_changes;
            precsaid = sp.stop_area_idx;
        } else if(this->is_stop_point(arrival) && this->is_stop_point(preds[arrival])) {
            const type::StopPoint & sp = data.stop_points[arrival - stop_point_offset];

            PathItem item(sp.stop_area_idx, distance[arrival], distance[arrival],
                          data.lines.size());
            result.items.push_back(item);

        }


        arrival = preds[arrival];
    }
    std::reverse(result.items.begin(), result.items.end());
    if(result.items.size() > 0)
        result.duration = result.items.back().arrival - result.items.front().departure;
    else
        result.duration = 0;
    return result;
}

}}}
