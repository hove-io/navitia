#include "benchmark.h"

int main(int , char **) {
    navitia::routing::benchmark::benchmark b("/home/vlara/navitiagit/build/routing/IdF", "/home/vlara/navitia/jeu/IdF/IdF.nav");

        b.load_input();



        {
            Timer t("Compute Raptor");
            b.computeBench_ra();
        }

}
