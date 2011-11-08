#include "types.h"

using namespace navitia::streetnetwork;
using navitia::type::idx_t;
int main(int, char**){
    StreetNetwork sn;
//    sn.load_bd("/home/tristram/BD_TOPO_STIF");
    sn.load_bin("/home/tristram/idf_street_network.nav");
    std::cout << "chargement fini" << std::endl;
 //   sn.save_bin("/home/tristram/idf_street_network.nav");
//    std::cout << "Sauvegarde finie" << std::endl;

    std::cout << "Construction de pl " << std::flush;
    sn.build_proximity_list();
    std::cout << "done" << std::endl;

    auto a = sn.fl.find("fontaine au roi");
    auto b = sn.fl.find("bd poniatowski");
    Way w1 = sn.ways[a[0]];
    Way w2 = sn.ways[b[0]];
    auto res = sn.compute({w1.edges[0].first}, {w2.edges[1].first});
    BOOST_FOREACH(auto item, res.path_items){
        std::cout << sn.ways[item.way_idx].name << " sur " << item.length << " mètres " << item.way_idx << std::endl;
    }

    std::cout << " Deuxième test ! " << std::endl;
    vertex_t source = sn.pl.find_nearest(2.4017, 48.8595); // Rue des pyrénées/rue de bagnolet
    vertex_t target = sn.pl.find_nearest(2.4017, 48.8595); // Rue louis blanc
    res = sn.compute({source}, {target});
    BOOST_FOREACH(auto item, res.path_items){
        std::cout << sn.ways[item.way_idx].name << " sur " << item.length << " mètres " << item.way_idx << std::endl;
    }

}
