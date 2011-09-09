#include "data.h"
#include <iostream>
#include <set>
#include <boost/graph/adjacency_list.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
using namespace navitia::type;
using namespace std;
namespace pt = boost::posix_time;

struct Edge{
    int duration;
};

struct Node{
    idx_t stop_point;
    idx_t line;
};

struct SortByOrder{
    Data & data;
    SortByOrder(Data&data) : data(data) {}
    bool operator()(idx_t a, idx_t b) {
        return data.stop_times[a].order < data.stop_times[b].order;
    }
};

int main(int , char ** )
{
    Data d;
/*
    d.load_flz("idf_flz.nav");

    std::cout
        << "Statistiques :" << std::endl
        << "    Nombre de StopAreas : " << d.stop_areas.size() << std::endl
        << "    Nombre de StopPoints : " << d.stop_points.size() << std::endl
        << "    Nombre de lignes : " << d.lines.size() << std::endl
        << "    Nombre d'horaires : " << d.stop_times.size() << std::endl << std::endl;

    std::map<int, int> rp;


    BOOST_FOREACH(StopTime st, d.stop_times) {
        idx_t idx = d.stop_points[st.stop_point_idx].stop_area_idx;
        idx = st.stop_point_idx;
        if(rp.find(idx) == rp.end())
            rp[idx] = 1;
        else rp[idx]++;
    }

    std::cout << "taille de rp " << rp.size() << std::endl;

    std::pair<int,int> pair;
    int zero = 0;
    int one = 0;
    int two = 0;
    int rest = 0;
    BOOST_FOREACH(pair, rp){
        if(pair.second == 0)
            zero++;
        if(pair.second == 1)
            one++;
        else if(pair.second == 2)
            two++;
        else
            rest++;
    }

    std::set<int> sa_sp;
    BOOST_FOREACH(StopPoint sp, d.stop_points) {
        sa_sp.insert(sp.stop_area_idx);
    }

    std::cout << "Nombre de stop areas " << sa_sp.size() << std::endl;

    std::cout << "Zero: " << zero << " - One: " << one << " - two:" << two << " - rest:" << rest << std::endl;


    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Node, Edge > Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

    Graph g(d.stop_times.size());
    std::cout << boost::num_vertices(g) << " nœuds" << std::endl;

    // On regroupe les stop_time par VJ et par order
    std::vector< std::vector<idx_t> > st_by_vj(d.vehicle_journeys.size());
    std::vector< std::vector<idx_t> > st_by_sp(d.stop_areas.size());
    std::cout << "Num de vj: " << st_by_vj.size() << std::endl;

    BOOST_FOREACH(StopTime st, d.stop_times) {
        st_by_vj[st.vehicle_journey_idx].push_back(st.idx);
        st_by_sp[d.stop_points[st.stop_point_idx].stop_area_idx].push_back(st.idx);
        g[st.idx].stop_point = st.stop_point_idx;
    }

    SortByOrder sort(d);

    int cnt = 0;
    // On rajoute les liens d'un meme parcours 
    BOOST_FOREACH(std::vector<idx_t> & stop_times, st_by_vj) {
        std::sort(stop_times.begin(), stop_times.end(), sort);

        for(size_t i = 1; i < stop_times.size(); i++){
            Edge e;
            e.duration = d.stop_times[stop_times[i]].arrival_time - d.stop_times[stop_times[i-1]].departure_time;
            boost::add_edge( stop_times[i-1],stop_times[i], e, g);
            cnt++;
        }
    }

    std::cout << "step 1 " << cnt << std::endl;
    cnt = 0;
    // On rajoute les correspondances entre stop_points
//    BOOST_FOREACH(std::vector<idx_t> & stop_times, st_by_sp) {
    for(size_t sa_idx = 0; sa_idx < st_by_sp.size(); sa_idx++){
        std::vector<idx_t> & stop_times = st_by_sp[sa_idx];
        std::sort(stop_times.begin(), stop_times.end(), sort);
        std::cout << "Stop area " << stop_times.size() <<  " "  << d.stop_areas[sa_idx].name << " - " << sa_idx << "/" << st_by_sp.size() <<  std::endl;
        for(size_t i=0; i < stop_times.size(); i++)
        {
            StopTime st1 = d.stop_times[stop_times[i]];
            std::vector<idx_t> reached_vj;
            for(size_t j=i+1; j < stop_times.size(); j++) 
            {
                StopTime st2 = d.stop_times[stop_times[j]];
                if(std::find(reached_vj.begin(), reached_vj.end(), d.vehicle_journeys[st2.vehicle_journey_idx].route_idx) == reached_vj.end())
                {
                    reached_vj.push_back(d.vehicle_journeys[st2.vehicle_journey_idx].route_idx);
                    Edge e;
                    e.duration = st2.arrival_time - st1.departure_time; 
                    if(e.duration > 120 && e.duration << 1200 && st2.vehicle_journey_idx != st1.vehicle_journey_idx){
                        boost::add_edge( stop_times[i],stop_times[j], e, g);
                        cnt++;
                        if(cnt % 10000 == 0)
                            std::cout << cnt << std::endl;
                    }
                }
            }
        }
    }

    std::cout << "step 2 " << cnt << std::endl;


    std::vector<vertex_t> p(boost::num_vertices(g));
    std::vector<int> di(boost::num_vertices(g));


    pt::ptime start(pt::microsec_clock::local_time());
    boost::dijkstra_shortest_paths(g, st_by_sp[5261][0], boost::predecessor_map(&p[0]).distance_map(&di[0]).weight_map(boost::get(&Edge::duration,g)));
    pt::ptime end = pt::microsec_clock::local_time();
    std::cout << "Temps de calcul " << (end-start).total_milliseconds() << " ms" << std::endl;

    int count = 0;
    int count2 = 0;
    BOOST_FOREACH(vertex_t v, p){
        if(p[v] != v)
            count++;

    }/
    std::cout << " On a " << count << " itinéraires" << std::endl;
    */return 0;
}
