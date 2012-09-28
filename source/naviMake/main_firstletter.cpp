#include "firstletterparser.h"


int main(int, char ** params) {
    navimake::Data data; // Structure temporaire
    navitia::type::Data nav_data; // Structure définitive

    navimake::connectors::FirstletterParser parser(params[1]);



    parser.load(data);
    data.sort();
    data.transform(nav_data.pt_data);
    nav_data.build_external_code();

    parser.load_address(nav_data);

    nav_data.build_first_letter();

    //std::cout << "taille fl streetnetwork : " << nav_data.street_network.fl.vec_map.size() << " nb ways : " <<  nav_data.street_network.ways.size() << std::endl;
    std::cout << "taille fl streetnetwork : " << nav_data.geo_ref.fl.vec_map.size() << " nb ways : " <<  nav_data.geo_ref.ways.size() << std::endl;

    nav_data.lz4(params[2]);
}
