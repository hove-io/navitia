#include "time_dependent.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/foreach.hpp>
namespace navitia { namespace routing {


DateTime operator+(DateTime dt, int seconds) {
    if(!(dt == DateTime::infinity())){
        dt.hour += seconds;
        //dt.normalize();
    }
    return dt;
}

std::ostream & operator<<(std::ostream & os, const DateTime & dt){
    os << "D=" << dt.date << " " << dt.hour/(3600) << ":" << (dt.hour%3600)/60;
    return os;
}

TimeDependent::TimeDependent(const type::PT_Data & data) : data(data),
       // On crée un nœud par route point, deux par stopArea, deux par stopPoint
       graph(data.route_points.size() + data.stop_areas.size() + data.stop_points.size()),
       stop_area_offset(data.route_points.size()),
       stop_point_offset(stop_area_offset + data.stop_areas.size())
   {
    BOOST_ASSERT(boost::num_vertices(this->graph) == stop_point_offset + data.stop_points.size());
    std::cout <<  3*data.route_points.size() - data.routes.size() + 2*data.stop_points.size() << std::endl;
    //BOOST_ASSERT(boost::num_edges(this->graph) == 3*data.route_points.size() - data.vehicle_journeys.size() + 2*data.stop_points.size() );
    }

void  TimeDependent::build_graph(){
    // 1. On rajoute un arc pour chaque segment de route
    BOOST_FOREACH(type::Route route, data.routes){
        for(size_t i = 1; i < route.route_point_list.size(); ++i){
            boost::add_edge(route.route_point_list[i-1], route.route_point_list[i], graph);
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
        boost::add_edge(route_point.stop_point_idx + stop_point_offset, route_point.idx, Edge(120), graph);
        // 0 à l'arrivée
        boost::add_edge(route_point.idx, route_point.stop_point_idx + stop_point_offset, Edge(1), graph);
    }

    // 4. On rajoute les horaires
    BOOST_FOREACH(type::VehicleJourney vj, data.vehicle_journeys){
        for(size_t i = 1; i < vj.stop_time_list.size(); ++i){
            type::StopTime stop1 = data.stop_times[vj.stop_time_list[i-1]];
            type::StopTime stop2 = data.stop_times[vj.stop_time_list[i]];
            ValidityPatternTime vpt1(vj.validity_pattern_idx, stop1.departure_time);
            ValidityPatternTime vpt2(vj.validity_pattern_idx, stop2.arrival_time);

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

    if(departure == DateTime::infinity())
        return departure;

    if(this->constant_duration >= 0)
        return departure + constant_duration;

    DateTime next_day;
    // Todo : optimiser par dichotomie car les time_tables sont triées par heure de départ
    BOOST_FOREACH(auto pair, this->time_table){
        ValidityPatternTime dep = pair.first;
        if(dep.vp_idx > data.validity_patterns.size())
            std::cout << "WTF! " << dep.vp_idx << std::endl;
        type::ValidityPattern vp = data.validity_patterns[dep.vp_idx];
        // On garde la première date du lendemain si jamais on ne trouve rien
        if(next_day == DateTime::infinity() && vp.check(departure.date + 1)){
            next_day.date = departure.date + 1;
            next_day.hour = pair.second.hour;
        }

        // Aha ! on a trouvé le premier départ le bon jour !
        if(departure.date < 0) std::cout << "dep neg" << std::endl;
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
    int result = std::numeric_limits<int>::max();
    BOOST_FOREACH(auto pair, this->time_table){
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
};

struct edge_less{
    bool operator()(DateTime a, DateTime b) const {
        return a < b;
    }
    bool operator ()(const TimeTable&, DateTime) const{return false;}
};

std::vector<PathItem> TimeDependent::compute(const type::StopArea &dep, const type::StopArea &arr, int hour, int day){
    std::vector<vertex_t> preds(boost::num_vertices(this->graph));
    std::vector<DateTime> distance(boost::num_vertices(this->graph));

    DateTime start_time;
    start_time.date = day;
    start_time.hour = hour;

    boost::dijkstra_shortest_paths(this->graph, dep.idx + stop_area_offset,
                                   boost::distance_map(&distance[0])
            .predecessor_map(&preds[0])
            .weight_map(boost::get(&Edge::t, graph))
            .distance_combine(Combine(this->data))
            .distance_inf(DateTime::infinity())
            .distance_zero(start_time)
            .distance_compare(edge_less())
            );

    std::vector<PathItem> result;
    vertex_t arrival = arr.idx + stop_area_offset;
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

// euclidean distance heuristic
/*class distance_heuristic : public astar_heuristic<Graph, int>
{
public:
  distance_heuristic(Graph & graph, vertex_t goal)
    : m_goal(goal) {}
  CostType operator()(Vertex u)
  {
    CostType dx = m_location[m_goal].x - m_location[u].x;
    CostType dy = m_location[m_goal].y - m_location[u].y;
    return ::sqrt(dx * dx + dy * dy);
  }
private:
  LocMap m_location;
  Vertex m_goal;
};


// visitor that terminates when we find the goal
template <class vertex_t>
class astar_goal_visitor : public boost::default_astar_visitor
{
public:
  astar_goal_visitor(vertex_t goal) : m_goal(goal) {}
  template <class Graph>
  void examine_vertex(vertex_t u, Graph& g) {
    if(u == m_goal)
      throw found_goal();
  }
private:
  vertex_t m_goal;

};
*/



}}
