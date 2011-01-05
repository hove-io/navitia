#include "gtfs_parser.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>


int main(int, char **) {
   /* GtfsParser p("/home/kinou/workspace/gtfs", "20101101");
    Data data = p.getData();
    data.build_index();
    
   

    
    BOOST_FOREACH(auto sp, data.stoppoint_of_stoparea.get(42)){
       std::cout << sp->name << " " << sp->code << " " << sp->stop_area_idx << std::endl;
    }

    std::string foo("1");
    data.find(&StopArea::code, "1");
    data.find(&StopPoint::stop_area_idx, 1);
    BOOST_FOREACH(auto sp, data.find(&StopPoint::name, "Le Petit Bois")){
       std::cout << sp->name << " " << sp->code << " " << sp->stop_area_idx << std::endl;
    }

    {
    std::ofstream ofs("data.nav");
    boost::archive::binary_oarchive oa(ofs);
    oa << data;
    }*/
    std::ifstream ifs("data.nav");
    boost::archive::binary_iarchive ia(ifs);
    Data data2;
    ia >> data2;
    


    BOOST_FOREACH(auto sp, data2.stoppoint_of_stoparea.get(42)){
       std::cout << sp->name << " " << sp->code << " " << sp->stop_area_idx << std::endl;
    }

    data2.find(&StopArea::code, "1");
    data2.find(&StopPoint::stop_area_idx, 1);
    BOOST_FOREACH(auto sp, data2.find(&StopPoint::name, "Le Petit Bois")){
       std::cout << sp->name << " " << sp->code << " " << sp->stop_area_idx << std::endl;
    }

    auto sa = data2.stop_area_by_name.get(0);
    auto sa2 = data2.stop_area_by_name.get(data2.stop_areas.size() -1);
    std::cout << sa.name << " " << sa2.name << " " << sa2.code << std::endl;
}
