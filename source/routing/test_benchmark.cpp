#include "benchmark.h"

int main(int , char **) {

    navitia::routing::benchmark::benchmark b("IdF666", "/home/vlara/navitia/jeu/IdF/IdF.nav");
    b.generate_input();


    {
        Timer tg("Benchmarks");
        b.computeBench_ra();
    }

}
