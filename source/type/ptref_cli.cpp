#include <iostream>
#include "ptreferential.h"
#include "data.h"



using namespace navitia::type;

int main(int argc, char** argv){
    std::cout << "Chargement des données..." << std::flush;
    Data d;
    d.load_flz("data.nav.flz");
    std::cout << " effectué" << std::endl << std::endl;

    std::cout
            << "Statistiques :" << std::endl
            << "    Nombre de StopAreas : " << d.stop_areas.size() << std::endl
            << "    Nombre de StopPoints : " << d.stop_points.size() << std::endl
            << "    Nombre de lignes : " << d.lines.size() << std::endl
            << "    Nombre d'horaires : " << d.stop_times.size() << std::endl << std::endl;


    if(argc != 2)
        std::cout << "Il faut exactement un paramètre" << std::endl;
    else {
        pbnavitia::PTRefResponse result = query(argv[1], d);
        std::cout << "octets généré en protocol buffer: " << result.ByteSize() << std::endl;
        std::cout << pb2txt(result);
        /*std::cout << "Il y a " << result.size() << " lignes de résultat" << std::endl;
        BOOST_FOREACH(auto row, result){
            BOOST_FOREACH(auto col, row){
                std::cout << col << ",\t";
            }
            std::cout << std::endl;
        }*/
    }
    return 0;
}
