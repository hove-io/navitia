#include "time_dependent.h"
#include "type/data.h"
#include "utils/timer.h"
using namespace navitia;

int main(int, char**){

    type::Data data;
    {
        Timer t("Chargement des données");
        data.load_lz4("IdF.nav");
        std::cout << "Num RoutePoints : " << data.pt_data.route_points.size() << std::endl;
    }

    routing::TimeDependent td(data.pt_data);
    {
        Timer t("Constuction du graphe");
        td.build_graph();
        std::cout << "Num nodes: " <<  boost::num_vertices(td.graph) << ", num edges: " << boost::num_edges(td.graph) << std::endl;
    }


    {
        Timer t("Calcul d'itinéraire");


        // Pont de neuilly : 13135
        // Gallieni 5980
        // Porte maillot 13207
        // bérault 1878
        // buzenval 1866
        /*auto l = td.compute(data.pt_data.stop_areas[1866], data.pt_data.stop_areas[5980]);
        for(auto s : l){
            std::cout << s << std::endl;
        }*/
    }
}
