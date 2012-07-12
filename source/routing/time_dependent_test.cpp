#include "time_dependent.h"
#include "type/data.h"
#include "utils/timer.h"
#include <time.h>
using namespace navitia;


void benchmark(routing::timedependent::TimeDependent & td, type::Data & data, int runs){
    srand ( time(NULL) );
    std::cout << "rand max : " << RAND_MAX << std::endl;
    Timer t;
    std::cout << "Depart;Arrivée;temps de calcul;distance à vol d'oiseau;% visité" << std::endl;
    for(int i=0; i < runs; ++i){
        type::idx_t dep = rand() % data.pt_data.stop_areas.size();
        type::idx_t arr = rand() % data.pt_data.stop_areas.size();
        int day = rand() % 10;
        int hour = rand() % (24*3600);
        type::StopArea sa_dep = data.pt_data.stop_areas[dep];
        type::StopArea sa_arr = data.pt_data.stop_areas[arr];
        t.reset();
        auto res = td.compute(dep, arr, hour, day);
        std::cout <<sa_dep.external_code << ";" << sa_arr.external_code << ";" << t.ms() << ";" << sa_dep.coord.distance_to(sa_arr.coord) << ";" << res.percent_visited << std::endl;
    }
}

int main(int, char**){
    type::Data data;
    {
        Timer t("Chargement des données");
        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");
        std::cout << "Num RoutePoints : " << data.pt_data.route_points.size() << std::endl;
        int count = 0;
        BOOST_FOREACH(auto sp, data.pt_data.stop_points){
            if(sp.route_point_list.size() <= 1)
                count++;
        }
        std::cout << count << " stop points avec un seul route point sur " << data.pt_data.stop_points.size() << std::endl;
    }

    routing::timedependent::TimeDependent td(data.pt_data);
    {
        Timer t("Constuction du graphe");
        td.build_graph();

//        td.build_heuristic(data.pt_data.stop_areas[142].idx);
//        td.compute_astar(data.pt_data.stop_areas[1], data.pt_data.stop_areas[142], 28800, 2);
        std::cout << "Num nodes: " <<  boost::num_vertices(td.graph) << ", num edges: " << boost::num_edges(td.graph) << std::endl;
    }

    {
        Timer t("Calcul itinéraire");
        std::cout << data.pt_data.stop_areas.at(312).name << " à " << data.pt_data.stop_areas.at(566).name << std::endl;
        std::cout << td.makeItineraire(td.compute(14796, 2460, 72000, 7));
    }





    {
    //    Timer t("Calcul d'itinéraire");

/*
        // Pont de neuilly : 13135
        // Gallieni 5980
        // Porte maillot 13207
        // bérault 1878
        // nation 12498
        // buzenval 1142
        bool b;
        routing::edge_t e;

        boost::tie(e, b) = boost::edge(188175, 188176, td.graph);
        BOOST_ASSERT(b);
        std::cout << td.graph[e].t.time_table.size() << std::endl;
        routing::DateTime start_time;
        start_time.hour = 3600*8;
        start_time.date = 8;

        auto res = td.graph[e].t.eval(start_time, td.data);
        std::cout << start_time << " " << res << std::endl;

        std::set<type::idx_t> moo;
        for(auto x : td.graph[e].t.time_table){
            if(x.first.hour > start_time.hour){
                moo.insert(x.first.vp_idx);
            }
        }
        for(auto x : moo ){
            std::cout << x << " " << data.pt_data.validity_patterns[x].days << std::endl;
        }


        boost::tie(e, b) = boost::edge(188213, 188214, td.graph);
        BOOST_ASSERT(e);
        std::cout << td.graph[e].t.time_table.size() << std::endl;

        boost::tie(e, b) = boost::edge(188282, 188283, td.graph);
        BOOST_ASSERT(e);
        std::cout << td.graph[e].t.time_table.size() << std::endl;
*/

        /*auto l = td.compute(data.pt_data.stop_areas[1142], data.pt_data.stop_areas[12498], 8000, 8);
        for(auto s : l){
            std::cout << s.stop_point_name << std::endl;
        }

        l = td.compute(data.pt_data.stop_areas[12498], data.pt_data.stop_areas[1142], 8000, 8);
        for(auto s : l){
            std::cout << s.stop_point_name << " " << s.time << " " << s.day << std::endl;
        }*/

//        int runs;
//        if(argc == 2) runs = atoi(argv[1]);
//        else runs = 100;
//        benchmark(td, data, runs);
    }
}
