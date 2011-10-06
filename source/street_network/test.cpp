#include "types.h"

using namespace navitia::streetnetwork;
int main(int, char**){
    StreetNetwork sn;
    sn.load_bd("/home/kinou/workspace/navitiacpp/build_debug/street_network/BD_TOPO_STIF");
}
