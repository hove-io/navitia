#pragma once

#include "routing/raptor.h"
#include "georef/street_network.h"
#include "type/type.pb.h"

#include <memory>

namespace navitia {

class Worker {
    private:
        std::unique_ptr<navitia::routing::raptor::RAPTOR> calculateur;
        std::unique_ptr<navitia::streetnetwork::StreetNetwork> street_network_worker;
        navitia::type::Data & data;

        log4cplus::Logger logger;
        boost::posix_time::ptime last_load_at;

    public:
        Worker(navitia::type::Data & data) : data(data), logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"))){}

        pbnavitia::Response dispatch(const pbnavitia::Request & request);

        type::GeographicalCoord coord_of_address(const type::EntryPoint & entry_point);

        void init_worker_data();

        pbnavitia::Response load();
        pbnavitia::Response status();
        pbnavitia::Response metadatas();
        pbnavitia::Response autocomplete(const pbnavitia::AutocompleteRequest &request);
        pbnavitia::Response next_stop_times(const pbnavitia::NextStopTimeRequest &request, pbnavitia::API api);
        pbnavitia::Response proximity_list(const pbnavitia::ProximityListRequest &request);
        pbnavitia::Response journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api);
        pbnavitia::Response pt_ref(const pbnavitia::PTRefRequest &request);
};

}
