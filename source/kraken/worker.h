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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

// forward declare
namespace navitia {
namespace routing {
struct RAPTOR;
}
}  // namespace navitia

#include "georef/street_network.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "type/accessibility_params.h"
#include "kraken/data_manager.h"
#include "utils/logger.h"
#include "kraken/configuration.h"
#include "type/pb_converter.h"

#include <memory>
#include <limits>

namespace navitia {

struct Deadline;

struct JourneysArg {
    type::EntryPoints origins;
    type::AccessibiliteParams accessibilite_params;
    std::vector<std::string> forbidden;
    std::vector<std::string> allowed;
    type::RTLevel rt_level = type::RTLevel::Base;
    type::EntryPoints destinations;
    std::vector<uint64_t> datetimes;
    boost::optional<type::EntryPoint> isochrone_center;
    JourneysArg(type::EntryPoints origins,
                type::AccessibiliteParams accessibilite_params,
                std::vector<std::string> forbidden,
                std::vector<std::string> allowed,
                type::RTLevel rt_level,
                type::EntryPoints destinations,
                std::vector<uint64_t> datetimes,
                boost::optional<type::EntryPoint> isochrone_center);
    JourneysArg();
};

class Worker {
private:
    std::unique_ptr<navitia::routing::RAPTOR> planner;
    std::unique_ptr<navitia::georef::StreetNetwork> street_network_worker;

    const kraken::Configuration conf;
    log4cplus::Logger logger;
    size_t last_data_identifier =
        std::numeric_limits<size_t>::max();  // to check that data did not change, do not use directly
    boost::posix_time::ptime last_load_at;

public:
    navitia::PbCreator pb_creator;

    Worker(kraken::Configuration conf);
    // we override de destructor this way we can forward declare Raptor
    // see: https://stackoverflow.com/questions/6012157/is-stdunique-ptrt-required-to-know-the-full-definition-of-t
    ~Worker();

    void dispatch(const navitia::Deadline& deadline, const pbnavitia::Request& request, const nt::Data& data);
    boost::optional<size_t> get_raptor_next_st_cache_miss() const;

private:
    void init_worker_data(const navitia::type::Data* data,
                          const pt::ptime now,
                          const pt::time_period action_period,
                          const bool disable_geojson = false,
                          const bool disable_feedpublisher = false,
                          const bool disable_disruption = false);

    void metadatas();
    void feed_publisher();
    void status();
    void geo_status();
    void autocomplete(const pbnavitia::PlacesRequest& request);
    void place_uri(const pbnavitia::PlaceUriRequest& request);
    void next_stop_times(const pbnavitia::NextStopTimeRequest& request, pbnavitia::API api);
    void proximity_list(const pbnavitia::PlacesNearbyRequest& request);

    JourneysArg fill_journeys(const pbnavitia::JourneysRequest& request);
    void err_msg_isochron(navitia::PbCreator& pb_creator, const std::string& err_msg);
    void journeys(const pbnavitia::JourneysRequest& request,
                  pbnavitia::API api,
                  const boost::posix_time::ptime& current_datetime);
    void isochrone(const pbnavitia::JourneysRequest& request);
    void pt_ref(const pbnavitia::PTRefRequest& request);
    void traffic_reports(const pbnavitia::TrafficReportsRequest& request);
    void line_reports(const pbnavitia::LineReportsRequest& request);
    void calendars(const pbnavitia::CalendarsRequest& request);
    void pt_object(const pbnavitia::PtobjectRequest& request);
    void place_code(const pbnavitia::PlaceCodeRequest& request);
    void nearest_stop_points(const pbnavitia::NearestStopPointsRequest& request);
    bool set_journeys_args(const pbnavitia::JourneysRequest& request, JourneysArg& arg, const std::string& name);
    void graphical_isochrone(const pbnavitia::GraphicalIsochroneRequest& request);
    void heat_map(const pbnavitia::HeatMapRequest& request);
    void car_co2_emission_on_crow_fly(const pbnavitia::CarCO2EmissionRequest& request);
    void direct_path(const pbnavitia::Request& request);

    /*
     * Given N origins and M destinations and street network mode, it returns a NxM matrix which contains durations
     * from origin to destination by taking street network
     * */
    void street_network_routing_matrix(const pbnavitia::StreetNetworkRoutingMatrixRequest& request);
    void odt_stop_points(const pbnavitia::GeographicalCoord& request);

    void get_matching_routes(const pbnavitia::MatchingRoute&);
    void equipment_reports(const pbnavitia::EquipmentReportsRequest& equipment_reports);
};

type::EntryPoint make_sn_entry_point(const std::string& place,
                                     const std::string& mode,
                                     const float speed,
                                     const int max_duration,
                                     const navitia::type::Data& data);

}  // namespace navitia
