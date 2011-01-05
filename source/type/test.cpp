#include "gtfs_parser.h"


int main(int, char **) {
    GtfsParser p("/home/kinou/workspace/gtfs", "20101101");
    Data data = p.getData();
    data.build_index();
    BOOST_FOREACH(auto sp, data.stoppoint_of_stoparea.get(42)){
        std::cout << sp->name << " " << sp->code << " " << sp->stop_area_idx << std::endl;
    }
}
