#include "benchmark.h"

int main(int , char **) {

        navitia::routing::benchmark::benchmark b("IdF3", "/home/vlara/navitia/jeu/IdF/IdF.nav");


    {
        Timer t("Generation des données");
        b.generate_input(0,5);
    }


    {
        Timer tg("Benchmarks");
        b.computeBench();
    }

}
