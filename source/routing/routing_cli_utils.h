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
        std::string start, target, date, first_section_mode, last_section_mode;
        po::variables_map vm;
        std::unique_ptr<nr::RAPTOR> raptor;
        nt::Data data;

        compute_options() : desc("Simple journey computation"){
            // clang-format off
            desc.add_options()
            ("start,s", po::value<std::string>(&start), "uri of point to start")
            ("target,t", po::value<std::string>(&target), "uri of point to end")
            ("date,d", po::value<std::string>(&date), "yyyymmddThhmmss")
            ("counterclockwise,c", "Counter-clockwise search")
            ("firstsectionmode,f", po::value<std::string>(&first_section_mode), "can be walking, bike, car, bss")
            ("lastsectionmonde,l", po::value<std::string>(&last_section_mode), "can be walking, bike, car, bss")
            ("protobuf,p", "Full-output");
            // clang-format on
        }
        bool compute();
        void load(const std::string &file);
        private:
        void show_section(const pbnavitia::Section& section);
    };
}}
