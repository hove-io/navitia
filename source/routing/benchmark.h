#pragma once
#include "string.h"
#include "type/type.h"
#include "type/data.h"
#include "boost/random.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "boost/random/uniform_int.hpp"
#include <boost/program_options.hpp>
#include "time_dependent.h"
#include "time_expanded.h"
#include "raptor.h"
#include "utils/timer.h"


namespace navitia { namespace routing { namespace benchmark {
typedef std::vector<int> vectorint;

struct benchmark
{
    std::string path;
    navitia::type::Data data;
    std::string pathInput;
    std::vector<vectorint> inputdatas;

    benchmark(std::string path, std::string pathData) : path(path), data() {
        data.load_lz4(pathData);
    }

    void generate_input();
    void generate_input(int date1, int date2);

    void load_input();

    void computeBench();
    void computeBench_td();
    void computeBench_tda();
    void computeBench_te();
    void computeBench_tea();
    void computeBench_ra();
    void testBench();

};


}}}

