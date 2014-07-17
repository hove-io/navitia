#include "routing_cli_utils.h"
#include "type/response.pb.h"
#include <boost/program_options.hpp>
#include "raptor.h"
#include "routing/raptor_api.h"

namespace nr = navitia::routing;
namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace po = boost::program_options ;
namespace pb = pbnavitia;

namespace navitia { namespace cli {

        bool compute_options::compute(const boost::program_options::variables_map& vm,
                nr::RAPTOR& raptor) {
            std::vector<std::string> forbidden;
            if (vm.count("start") == 0 || vm.count("target") == 0 || vm.count("date") == 0) {
                return false;
            }
            bool clockwise = !vm.count("counterclockwise");
            navitia::georef::StreetNetwork sn_worker(*raptor.data.geo_ref);
            
            nt::Type_e origin_type = raptor.data.get_type_of_id(start);
            nt::Type_e destination_type = raptor.data.get_type_of_id(target);
            nt::EntryPoint origin(origin_type, start);
            nt::EntryPoint destination(destination_type, target);
            pb::Response resp = make_response(raptor, origin, destination, {date},
                    clockwise, navitia::type::AccessibiliteParams(), forbidden,
                    sn_worker, false, true);

            if (vm.count("protobuf")) {
                std::cout << resp.DebugString() << "\n";
                return true;
            }
            std::cout << "Response type: " <<  pb::ResponseStatus_Name(pb::ResponseStatus(resp.response_type())) << "\n";
            if (resp.journeys_size() == 0) {
                std::cout << "No solutions found\n";
                return false;
            }
            for (int i = 0; i < resp.journeys_size();i++) {
                auto journey = resp.journeys(i);
                std::cout << "Journey " << journey.nb_transfers() << " transfers" << "\n";
                for (int j = 0; j < journey.sections_size(); j++) {
                    show_section(journey.sections(j));
                }
                std::cout << std::endl;
            }
        return true;
    }

    void compute_options::show_section(const pbnavitia::Section& section) {
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

}}
