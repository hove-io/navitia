/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "raptor.h"
#include "routing/raptor_api.h"
#include "type/data.h"
#include "utils/timer.h"
#include "type/response.pb.h"
#include <boost/program_options.hpp>
#include "routing_cli_utils.h"

namespace nr = navitia::routing;
namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace po = boost::program_options ;
namespace pb = pbnavitia;

int main(int argc, char** argv) {
    po::options_description desc("Simple journey computation");
    std::string file;
    navitia::cli::compute_options compute_opt;
    desc.add_options()
            ("help", "Show help")
            ("file,f", po::value<std::string>(&file)->default_value("Path to data.nav.lz4"));
    desc.add(compute_opt.desc);

    po::store(po::parse_command_line(argc, argv, desc), compute_opt.vm);
    po::notify(compute_opt.vm);

    if (compute_opt.vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (compute_opt.vm.count("file") == 0) {
        std::cerr << "file parameter is required";
    }
    compute_opt.load(file);
    compute_opt.compute();
}
