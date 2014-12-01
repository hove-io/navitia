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

//forward declare
namespace navitia{
namespace routing{
    struct RAPTOR;
}
}

#include "georef/street_network.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "kraken/data_manager.h"
#include "utils/logger.h"
#include "kraken/configuration.h"

#include <memory>

namespace navitia {

class Worker {
    private:
        std::unique_ptr<navitia::routing::RAPTOR> planner;
        std::unique_ptr<navitia::georef::StreetNetwork> street_network_worker;

        // we keep a reference to data_manager in each thread
        DataManager<navitia::type::Data>& data_manager;
        const kraken::Configuration conf;
        log4cplus::Logger logger;
        const void* last_data = nullptr;// to check that data did not change, do not use directly
        boost::posix_time::ptime last_load_at;

    public:
        Worker(DataManager<navitia::type::Data>& data_manager, kraken::Configuration conf);
        //we override de destructor this way we can forward declare Raptor
        //see: https://stackoverflow.com/questions/6012157/is-stdunique-ptrt-required-to-know-the-full-definition-of-t
        ~Worker();

        pbnavitia::Response dispatch(const pbnavitia::Request & request);

        type::GeographicalCoord coord_of_entry_point(const type::EntryPoint & entry_point,
                const boost::shared_ptr<const navitia::type::Data> data);
        type::StreetNetworkParams streetnetwork_params_of_entry_point(const pbnavitia::StreetNetworkParams & request, const boost::shared_ptr<const navitia::type::Data> data, const bool use_second = true);

        void init_worker_data(const boost::shared_ptr<const navitia::type::Data> data);

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
        pbnavitia::Response pt_object(const pbnavitia::PtobjectRequest &request);        
        pbnavitia::Response place_code(const pbnavitia::PlaceCodeRequest &request);
};

}
