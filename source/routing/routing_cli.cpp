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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise/linenoise.h"
#include "raptor.h"
#include "routing/raptor_api.h"
#include "type/data.h"
#include "utils/timer.h"
#include "type/response.pb.h"
#include "routing_cli_utils.h"
#include "ptreferential/ptreferential_api.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "type/pb_converter.h"

namespace nr = navitia::routing;
namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace po = boost::program_options ;
namespace pb = pbnavitia;

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'j' || buf[0] == '\0') {
        linenoiseAddCompletion(lc,"journey");
    }
    if (buf[0] == 'p' || buf[0] == '\0') {
        linenoiseAddCompletion(lc,"ptref");
    } 
}
/* This program takes a path to a nav.lz4 navitia's file
 * It will be loaded at the begginning of the program
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "A filename is needed" << std::endl;
        return 1;
    }

    navitia::cli::compute_options compute_opt;
    compute_opt.load(argv[1]);
    po::variables_map vm;
    char *line;
    /* Parse options, with --multiline we enable multi line editing. */
    linenoiseSetCompletionCallback(completion);
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */
    while((line = linenoise("hello> ")) != nullptr) {
        linenoiseHistoryAdd(line);
        linenoiseHistorySave("history.txt");
        std::string str_line(line);
        auto splitted_line = po::split_unix(str_line);
        if (splitted_line.empty()) {
            continue;
        }
        if (splitted_line[0] == "journey") {
            po::store(po::command_line_parser(splitted_line).options(compute_opt.desc).run(), compute_opt.vm);
            po::notify(compute_opt.vm);
            compute_opt.compute();
        } else if (splitted_line[0] == "ptref") {
            if (splitted_line.size() < 2) {
                std::cerr << "an ID is needed" << std::endl;
                continue;
            }
            const std::string id = splitted_line[1];
            #define SHOW_ID_CLI(type_name, collection_name) \
            auto collection_name##_map = compute_opt.data.pt_data->collection_name##_map;\
            if ( collection_name##_map.find(id) != collection_name##_map.end()) {\
                pbnavitia::type_name p;\
                navitia::fill_pb_object(collection_name##_map.at(id), compute_opt.data, &p);\
                std::cout << p.DebugString() << std::endl;}
            ITERATE_NAVITIA_PT_TYPES(SHOW_ID_CLI)
        }
    }
    return 0;
}
