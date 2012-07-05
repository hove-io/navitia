#include "time_dependent.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/foreach.hpp>
#include "utils/timer.h"
namespace navitia { namespace routing { namespace timedependent {

class distance_heuristic : public boost::astar_heuristic<Graph, DateTime>
{
public:
  distance_heuristic(const TimeDependent & td) : td(td){}

  // Estimation de l'heure d'arrivée. Cette heure doit est sous-estimée
  DateTime operator()(Vertex u){
      Vertex a = u;
      /*if(a > td.stop_area_offset)
          a -= td.stop_area_offset;
      else
          a = td.data.route_points[a].stop_point_idx;*/
      //BOOST_ASSERT(td.distance[u] == DateTime::infinity());
      DateTime result;
      result.hour = td.astar_graph.min_time.at(a);
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
       stop_area_offset(0),
       stop_point_offset(data.stop_areas.size()),
       route_point_offset(stop_point_offset + data.stop_points.size()),
       preds(boost::num_vertices(graph)),
       distance(boost::num_vertices(graph)),
       astar_dist(boost::num_vertices(graph))
   {
    BOOST_ASSERT(boost::num_vertices(this->graph) == stop_point_offset + data.stop_points.size());
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
        boost::add_edge(stop_point.stop_area_idx + stop_area_offset, stop_point.idx + stop_point_offset, Edge(60), graph);
        boost::add_edge(stop_point.idx + stop_point_offset, stop_point.stop_area_idx + stop_area_offset, Edge(60), graph);
    }


    // 3. On rajoute un arc entre route_points et stop_points de durée constante
    BOOST_FOREACH(type::RoutePoint route_point, data.route_points){
        // Deux minutes de battement à l'arrivée
        boost::add_edge(route_point.stop_point_idx + stop_point_offset, route_point.idx + route_point_offset, Edge(120), graph);
        // 0 à l'arrivée
        boost::add_edge(route_point.idx + route_point_offset, route_point.stop_point_idx + stop_point_offset, Edge(1), graph);
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

DateTime TimeTable::eval(DateTime departure, const type::PT_Data &data) const{
    BOOST_ASSERT(departure.date >= 0);
    if(departure == DateTime::infinity())
        return departure;

    if(this->constant_duration >= 0)
        return departure + constant_duration;

    DateTime next_day;
    // Todo : optimiser par dichotomie car les time_tables sont triées par heure de départ
    BOOST_FOREACH(auto pair, this->time_table){
        ValidityPatternTime dep = pair.first;
        if(dep.vp_idx == type::invalid_idx)
            std::cout << "CRAPY CRAP" << std::endl;
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
           // result.normalize();

            return result;
        }
    }
    return next_day;
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

bool operator==(const PathItem & a, const PathItem & b){
    return a.stop_point_name == b.stop_point_name && a.time == b.time && a.day == b.day;
}

std::ostream & operator<<(std::ostream & os, const PathItem & b){
    os << b.stop_point_name << " " << b.day << " " << b.time;
    return os;
}

Path TimeDependent::compute(type::idx_t dep, type::idx_t arr, int hour, int day){
    DateTime start_time;
    start_time.date = day;
    start_time.hour = hour;

    try{
        boost::dijkstra_shortest_paths(this->graph, dep + stop_area_offset,
                                       boost::distance_map(&distance[0])
                                       .predecessor_map(&preds[0])
                                       .weight_map(boost::get(&Edge::t, graph))
                                       .distance_combine(Combine(this->data))
                                       .distance_inf(DateTime::infinity())
                                       .distance_zero(start_time)
                                       .distance_compare(edge_less())
                                       .visitor(goal_visitor(arr + stop_area_offset))
                                       );
    } catch(found_goal){}

    Path result;


    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(preds[v] != v)
            count++;
    }

    result.percent_visited = 100*count/boost::num_vertices(this->graph);


    vertex_t arrival = arr + stop_area_offset;
    while(preds[arrival] != arrival){
        if(arrival >= this->route_point_offset){
            const type::StopPoint & sp = data.stop_points[data.route_points[arrival-route_point_offset].stop_point_idx];
            PathItem item;
            item.stop_point_name = sp.name;
            item.day = distance[arrival].date;
            item.time = distance[arrival].hour;
            result.items.push_back(item);
        }
        arrival = preds[arrival];
    }
    std::reverse(result.items.begin(), result.items.end());
    return result;
}


std::vector<routing::PathItem> TimeDependent::compute_astar(const type::StopArea &dep, const type::StopArea &arr, int hour, int day){
    DateTime start_time;
    start_time.date = day;
    start_time.hour = hour;

    vertex_t departure = dep.idx + stop_area_offset;
    vertex_t arrival = arr.idx + stop_area_offset;

//    astar_graph.build_heuristic(arrival);

    try{
        boost::astar_search(this->graph, departure, distance_heuristic(*this),
                            boost::distance_map(&distance[0])
                            .predecessor_map(&preds[0])
                            .weight_map(boost::get(&Edge::t, graph))
                            .distance_combine(Combine(this->data))
                            .distance_inf(DateTime::infinity())
                            .distance_zero(start_time)
                            .distance_compare(edge_less())
                            .visitor(navitia::routing::astar::astar_goal_visitor(arrival))
                            .rank_map(&astar_dist[0])
            );
    } catch(navitia::routing::astar::found_goal){}

    std::vector<routing::PathItem> result;

    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(preds[v] != v)
            count++;
    }
    std::cout << "A* : " << 100*count/boost::num_vertices(this->graph) << "%  ";

    while(preds[arrival] != arrival){
        if(arrival < data.route_points.size()){
            const type::StopPoint & sp = data.stop_points[data.route_points[arrival].stop_point_idx];
            PathItem item;
            item.stop_point_name = sp.name;
            item.day = distance[arrival].date;
            item.time = distance[arrival].hour;
            result.push_back(item);
        }
        arrival = preds[arrival];
    }
    std::reverse(result.begin(), result.end());
    return result;
}

}}}
