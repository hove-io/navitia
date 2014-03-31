#pragma once

//forward declare
namespace navitia{
namespace routing{
    class RAPTOR;
}
}

#include "georef/street_network.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "kraken/data_manager.h"
#include "utils/logger.h"

#include <memory>

namespace navitia {

class Worker {
    private:
        std::unique_ptr<navitia::routing::RAPTOR> planner;
        std::unique_ptr<navitia::georef::StreetNetwork> street_network_worker;

        // we keep a reference to data_manager in each thread
        DataManager<navitia::type::Data>& data_manager;

        log4cplus::Logger logger;
        boost::posix_time::ptime last_load_at;

    public:
        Worker(DataManager<navitia::type::Data>& data_manager);
        //we override de destructor this way we can forward declare Raptor
        //see: https://stackoverflow.com/questions/6012157/is-stdunique-ptrt-required-to-know-the-full-definition-of-t
        ~Worker();

        pbnavitia::Response dispatch(const pbnavitia::Request & request);

        type::GeographicalCoord coord_of_entry_point(const type::EntryPoint & entry_point,
                const std::shared_ptr<navitia::type::Data> data);
        type::StreetNetworkParams streetnetwork_params_of_entry_point(const pbnavitia::StreetNetworkParams & request, const std::shared_ptr<navitia::type::Data> data, const bool use_second = true);

        void init_worker_data(const std::shared_ptr<navitia::type::Data> data);

        pbnavitia::Response status();
        pbnavitia::Response metadatas();
        pbnavitia::Response autocomplete(const pbnavitia::PlacesRequest &request);
        pbnavitia::Response place_uri(const pbnavitia::PlaceUriRequest &request);
        pbnavitia::Response next_stop_times(const pbnavitia::NextStopTimeRequest &request, pbnavitia::API api);
        pbnavitia::Response proximity_list(const pbnavitia::PlacesNearbyRequest &request);
        pbnavitia::Response journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api);
        pbnavitia::Response pt_ref(const pbnavitia::PTRefRequest &request);
        pbnavitia::Response disruptions(const pbnavitia::DisruptionsRequest &request);
        pbnavitia::Response calendars(const pbnavitia::CalendarsRequest &request);
};

}
