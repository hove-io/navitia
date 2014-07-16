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

namespace nr = navitia::routing;
namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace po = boost::program_options ;
namespace pb = pbnavitia;

int main(int argc, char** argv){
    po::options_description desc("Options du calculateur simple");
    std::string start, target, date;
    std::string file;
    nt::Data data;
    desc.add_options()
            ("help", "Show help")
            ("start,s", po::value<std::string>(&start), "uri of point to start")
            ("target,t", po::value<std::string>(&target), "uri of point to end")
            ("date,d", po::value<std::string>(&date), "yyyymmddThhmmss")
            ("counterclockwise,c", "Counter-clockwise search")
            ("protobuf,p", "Full-output")
            ("file,f", po::value<std::string>(&file)->default_value("Path to data.nav.lz4"));

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    bool clockwise = !vm.count("counterclockwise");

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if(vm.count("start") && vm.count("target") && vm.count("date")) {
        {
            Timer t("Loading datafile : " + file);
            data.load(file);
        }
        nr::RAPTOR raptor(data);
        navitia::georef::StreetNetwork sn_worker(*data.geo_ref);
        std::vector<std::string> forbidden;

        nt::Type_e origin_type = data.get_type_of_id(start);
        nt::Type_e destination_type = data.get_type_of_id(start);
        nt::EntryPoint origin(origin_type, start);
        nt::EntryPoint destination(destination_type, target);
        pb::Response resp = make_response(raptor, origin, destination, {date}, clockwise, navitia::type::AccessibiliteParams()/*false*/, forbidden, sn_worker, false, true);

        if (vm.count("protobuf")){
            std::cout << resp.DebugString() << "\n";
        }else{
            std::cout << "Response type: " <<  pb::ResponseStatus_Name(pb::ResponseStatus(resp.response_type())) << "\n";
            if (resp.journeys_size() == 0){
                std::cout << "No solutions found\n";
            }
            for (int i = 0; i < resp.journeys_size();i++){
                auto journey = resp.journeys(i);
                std::cout << "Journey " << journey.nb_transfers() << " transfers" << "\n";
                for (int j = 0; j < journey.sections_size(); j++){
                    auto section = journey.sections(j);
                    std::cout << std::left << std::setw(18) << pb::SectionType_Name(pb::SectionType(section.type()));
                    std::cout << std::left << std::setw(16) << section.begin_date_time();
                    std::cout << std::left << std::setw(16) << section.end_date_time();
                    if (section.has_transfer_type()){
                        std::cout << std::left << std::setw(10) << pb::TransferType_Name(pb::TransferType(section.transfer_type()));
                    }else if (section.has_pt_display_informations()){
                        std::cout << std::left << std::setw(10) << section.pt_display_informations().physical_mode();
                    }else{
                        std::cout << std::left << std::setw(10) << "";
                    }
                    if (section.has_pt_display_informations()){
                        std::cout << std::left << std::setw(10)
                                  << section.pt_display_informations().code();
                        std::cout << std::left << std::setw(20)
                                  << section.pt_display_informations().headsign();
                        std::cout << std::left << std::setw(10)
                                  << section.pt_display_informations().network();
                    }else{
                        std::cout << std::left << std::setw(40) << "";
                    }
                    std::cout << std::left << std::setw(30) << section.origin().name();
                    std::cout << section.destination().name();
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
        return 0;
    } else {
        std::cout << desc << std::endl;
        return 1;
    }

}
