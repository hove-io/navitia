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

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "A filename is needed" ;
        return 1;
    }
    std::string start, target, date;
    nt::Data data;
    data.load(std::string(argv[1]));
    navitia::cli::compute_options compute_opt;
    po::variables_map vm;
    char *line;

    nr::RAPTOR raptor(data);
    /* Parse options, with --multiline we enable multi line editing. */
    linenoiseSetCompletionCallback(completion);
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */
    while((line = linenoise("hello> ")) != NULL) {
        linenoiseHistoryAdd(line);
        linenoiseHistorySave("history.txt");
        std::string str_line(line);
        auto splitted_line = po::split_unix(str_line);
        if (splitted_line.empty()) {
            continue;
        }
        if (splitted_line[0] == "journey") {
            po::store(po::command_line_parser(splitted_line).options(compute_opt.desc).run(), vm);
            po::notify(vm);
            compute_opt.compute(vm, raptor);
        } else if (splitted_line[0] == "ptref") {
            if (splitted_line.size() < 2) {
                std::cerr << "an ID is needed" << std::endl;
            }
            const std::string id = splitted_line[1];
            #define SHOW_ID_CLI(type_name, collection_name) \
            auto collection_name##_map = data.pt_data->collection_name##_map;\
            if ( collection_name##_map.find(id) != collection_name##_map.end()) {\
                pbnavitia::type_name p;\
                navitia::fill_pb_object(collection_name##_map.at(id), data, &p);\
                std::cout << p.DebugString() << std::endl;}
            ITERATE_NAVITIA_PT_TYPES(SHOW_ID_CLI)
        }
    }
    return 0;
}
