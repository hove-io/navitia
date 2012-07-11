#include "benchmark.h"

int main(int , char **) {

    navitia::routing::benchmark::benchmark b("poitiers", "/home/vlara/navitia/jeu/poitiers/poitiers.nav");
    b.generate_input();


    {
        Timer tg("Benchmarks");
        b.computeBench();
    }

}
