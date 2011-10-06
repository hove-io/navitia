#include "types.h"

using namespace navitia::streetnetwork;
int main(int, char**){
    StreetNetwork sn;
    sn.load_bdtopo("/home/tristram/Bureau/partage/bd_topo_stif/route_adresse.txt");
}
