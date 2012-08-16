#include "time_expanded.h"
#include "type/data.h"
#include "utils/timer.h"
#include <iostream>

#include <boost/graph/graphviz.hpp>

using namespace navitia;

struct label_name {
    routing::timeexpanded::TimeExpanded & te;

    label_name(routing::timeexpanded::TimeExpanded & te): te(te){}

    void operator()(std::ostream& oss,routing::timeexpanded::vertex_t v) {
        oss << "[label=\"";
        switch(te.get_n_type(v)) {
        case routing::timeexpanded::SA : oss << "SA " << te.get_idx(v) << te.data.stop_areas.at(v).name;  break;
        case routing::timeexpanded::SP : oss << "SP " << te.get_idx(v); break;
        case routing::timeexpanded::RP : oss << "RP " << te.get_idx(v); break;
        case routing::timeexpanded::TA : oss << "TA " << te.get_idx(v) << " " << te.get_time(v); break;
        case routing::timeexpanded::TD : oss << "TD " << te.get_idx(v) << " "<< te.get_time(v); break;
        case routing::timeexpanded::TC : oss << "TC " << te.get_idx(v) << " "<< te.get_time(v); break;
        }
        oss << " " << v << " " << te.get_saidx(v) << "\"]";
    }

};

int main(int /*argc*/, char** /*argv*/){
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

    routing::timeexpanded::TimeExpanded te(data.pt_data);
    {
        Timer t("Constuction du graphe");
        te.build_graph();
        te.astar_graph.build_graph();
        te.build_heuristic(973);
        std::cout << "Num nodes: " <<  boost::num_vertices(te.graph) << ", num edges: " << boost::num_edges(te.graph) << std::endl;
    }


    {
        Timer t("Dijkstra");
        std::cout << te.compute(13587, 973, 28800, 20);
    }
    {
        Timer t("Astar");

       std::cout <<  te.compute_astar(13587, 973, 28800, 20);

    }
}
