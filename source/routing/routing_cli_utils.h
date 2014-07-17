#pragma once
#include <boost/program_options.hpp>
#include "raptor.h"
#include "routing/raptor_api.h"

namespace nr = navitia::routing;
namespace po = boost::program_options ;
namespace pb = pbnavitia;

namespace navitia { namespace cli {

    struct compute_options {
        po::options_description desc;
        std::string start, target, date;
        po::variables_map vm;

        compute_options() : desc("Simple journey computation"){
            desc.add_options()
            ("start,s", po::value<std::string>(&start), "uri of point to start")
            ("target,t", po::value<std::string>(&target), "uri of point to end")
            ("date,d", po::value<std::string>(&date), "yyyymmddThhmmss")
            ("counterclockwise,c", "Counter-clockwise search")
            ("protobuf,p", "Full-output");
        }
        bool compute(const boost::program_options::variables_map& vm, nr::RAPTOR& raptor);
        private:
        void show_section(const pbnavitia::Section& section);
    };
}}
