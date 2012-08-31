#include "benchmark.h"

#include <boost/progress.hpp>

namespace navitia { namespace routing { namespace benchmark {

void benchmark::generate_input() {
    int fday = 7;
    if(data.pt_data.validity_patterns.front().beginning_date.day_of_week().as_number() == 6)
        generate_input(fday, 8);
    else
        generate_input(fday, 13 - data.pt_data.validity_patterns.front().beginning_date.day_of_week().as_number());
}

void benchmark::generate_input(int fday, int sday) {
    boost::mt19937 rng;
    boost::uniform_int<> sa(0,data.pt_data.stop_areas.size());
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > gen(rng, sa);

    std::fstream file(path+"/inputcsv", std::ios::out );
    for(int i = 0; i < 125; ++i) {
        int a = gen();
        int b = gen();
        while(b == a ) {
            b = gen();
        }

        file << a << "," << b << "," << 0 << "," << fday << "\n";
        file << a << "," << b << "," << 28800 << "," << fday << "\n";
        file << a << "," << b << "," << 72000 << "," << fday << "\n";
        file << a << "," << b << "," << 86000 << "," << fday << "\n";

        file << a << "," << b << "," << 0 << "," <<  sday << "\n";
        file << a << "," << b << "," << 28800 << "," << sday << "\n";
        file << a << "," << b << "," << 72000 << "," << sday << "\n";
        file << a << "," << b << "," << 86000 << "," << sday << "\n";

        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(0);inputdatas.back().push_back(fday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(28800);inputdatas.back().push_back(fday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(72000);inputdatas.back().push_back(fday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(86000);inputdatas.back().push_back(fday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(0);inputdatas.back().push_back(sday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(28800);inputdatas.back().push_back(sday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(72000);inputdatas.back().push_back(sday);
        inputdatas.push_back(vectorint());
        inputdatas.back().push_back(a);inputdatas.back().push_back(b);inputdatas.back().push_back(86000);inputdatas.back().push_back(sday);

    }
    file.close();
}

void benchmark::load_input() {
    typedef boost::tokenizer<boost::char_separator<char> >
            tokenizer;
    boost::char_separator<char> sep(",");

    std::fstream file(path+"/inputcsv", std::ios::in );

    if(file) {
        std::string line;
        while(std::getline(file, line)) {
            tokenizer tokens(line, sep);
            inputdatas.push_back(vectorint());
            for(tokenizer::iterator tok_iter = tokens.begin();
                tok_iter != tokens.end(); ++tok_iter) {
                inputdatas.back().push_back(boost::lexical_cast<int>(*tok_iter));
            }
        }
    } else {
        std::cout << "Erreur à l'ouverture " << std::endl;
    }

}

void benchmark::computeBench() {
    Timer tg("Timer General");
//    std::cout << std::endl << "Lancement TimeDepedent" << std::endl;
//    {
//        Timer t("Benchmark TimeDependent");
//        computeBench_td();

//    }
//    std::cout << std::endl << "Lancement TimeDepedent Astar" << std::endl;

//    {
//        Timer t("Benchmark TimeDependent Astar");
//        computeBench_tda();
//    }
//    std::cout << std::endl << "Lancement TimeExpanded" << std::endl;

//    {
//        Timer t("Benchmark TimeExpanded");
//        computeBench_te();
//    }
//    std::cout << std::endl << "Lancement TimeExpanded Astar" << std::endl;
//    {
//        Timer t("Benchmark TimeExpanded Astar");
//        computeBench_tea();
//    }
//    std::cout << std::endl << "Lancement RAPTOR" << std::endl;

//    {
//        Timer t("Benchmark RAPTOR");
//        computeBench_ra();
//    }

//    std::cout << std::endl << "Lancement RAPTOR rabattement" << std::endl;

//    {
//        Timer t("Benchmark RAPTOR rabattement");
//        computeBench_rab();
//    }

    std::cout << std::endl << "Lancement RAPTOR reverse" << std::endl;

    {
        Timer t("Benchmark RAPTOR rabattement");
        computeBench_re();
    }
}
void benchmark::computeBench_td() {
    routing::timedependent::TimeDependent td(data);
    td.build_graph();
    std::fstream file(path+"/td", std::ios::out );

    boost::progress_display show_progress(inputdatas.size());
    BOOST_FOREACH(auto entry, inputdatas) {
        //std::cout << count << std::flush;
        ++show_progress;
        Timer t;
        Path result = td.compute(entry[0], entry[1], entry[2], entry[3], partirapres);
        int temps = t.ms();
        if(result.items.size() > 0)
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour() <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
        else
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
    }
    std::cout << std::endl << std::endl;
    file.close();
}

void benchmark::computeBench_te() {
//    routing::timeexpanded::TimeExpanded te(data.pt_data);
//    te.build_graph();
//    std::fstream file(path+"/te", std::ios::out );

//    int count = 0;
//    BOOST_FOREACH(auto entry, inputdatas) {
//        ++count;
//        std::cout << count << std::flush;
//        Timer t;
//        Path result = te.compute(entry[0], entry[1], entry[2], entry[3]);
//        int temps = t.ms();
//        if(result.items.size() > 0)
//            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour <<"," << result.duration << ","
//                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
//        else
//            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
//                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
//    }
//    std::cout << std::endl << std::endl;
//    file.close();
}

void benchmark::computeBench_tea() {
//    routing::timeexpanded::TimeExpanded te(data.pt_data);
//    te.build_graph();
//    std::fstream file(path+"/tea", std::ios::out );

//    int count = 0;
//    BOOST_FOREACH(auto entry, inputdatas) {
//        ++count;
//        std::cout << count << std::flush;
//        Timer t;
//        te.build_heuristic(entry[1]);
//        Path result = te.compute_astar(entry[0], entry[1], entry[2], entry[3]);
//        int temps = t.ms();
//        if(result.items.size() > 0)
//            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour <<"," << result.duration << ","
//                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
//        else
//            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
//                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
//    }
//    std::cout << std::endl << std::endl;
//    file.close();
}
void benchmark::computeBench_ra() {
    routing::raptor::RAPTOR raptor(data);
    std::fstream file(path+"/ra", std::ios::out );

    int count = 0;
    BOOST_FOREACH(auto entry, inputdatas) {
        ++count;
        std::cout << count <<   std::flush;
        Timer t;
        Path result = raptor.compute(entry[0], entry[1], entry[2], entry[3], partirapres);
        int temps = t.ms();
        if(result.items.size() > 0)
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour() <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
        else
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
    }
    std::cout << std::endl << std::endl;

    file.close();
}

void benchmark::computeBench_re() {
    routing::raptor::RAPTOR raptor(data);
    std::fstream file(path+"/ra", std::ios::out );

    int count = 0;
    BOOST_FOREACH(auto entry, inputdatas) {
        ++count;
        std::cout << count <<   std::flush;
        Timer t;
        Path result = raptor.compute(entry[0], entry[1], entry[2], entry[3], arriveravant);
        int temps = t.ms();
        if(result.items.size() > 0)
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour() <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
        else
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
    }
    std::cout << std::endl << std::endl;

    file.close();
}

void benchmark::computeBench_rab() {
    routing::raptor::RAPTOR raptor(data);
    std::fstream file(path+"/ra", std::ios::out );

    int count = 0;
    BOOST_FOREACH(auto entry, inputdatas) {
        ++count;
        std::cout << count <<   std::flush;
        Timer t;
        Path result = raptor.compute_rabattement(entry[0], entry[1], entry[2], entry[3]);
        int temps = t.ms();
        if(result.items.size() > 0)
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << result.items.back().arrival.hour() <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
        else
            file << entry[0] << "," << entry[1] << "," << entry[2] << "," << entry[3] << ", " << -1 <<"," << result.duration << ","
                 << result.nb_changes << "," << result.percent_visited << "," << temps << std::endl;
    }
    std::cout << std::endl << std::endl;

    file.close();
}

void benchmark::testBench() {}



}}}
