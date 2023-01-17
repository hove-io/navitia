/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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

#include "utils/functions.h"
#include "type/data.h"
#include "georef/georef.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "type/message.h"
#include "type/rt_level.h"
#include "type/company.h"
#include "type/access_point.h"
#include "type/dataset.h"
#include "type/contributor.h"
#include "type/commercial_mode.h"

#include <boost/optional.hpp>
#include <boost/date_time/gregorian_calendar.hpp>

#include <memory>

/** Ce connecteur permet de faciliter la construction d'un réseau en code
 *
 *  Cela est particulièrement (voire exclusivement) utile pour les tests unitaires
 */

namespace ed {

struct builder;

// We store times as int to handle < 0 times (shifted into the negative because of boarding_duration)
struct ST {
    navitia::type::StopTime st;
    ST(navitia::type::StopTime stop_time) { st = stop_time; }

    int departure_time;
    int arrival_time;
    int boarding_time;
    int alighting_time;
};

/// Structure retournée à la construction d'un VehicleJourney
struct VJ {
    builder& b;
    const std::string network_name;
    const std::string line_name;
    std::string _block_id;
    std::string _route_name;
    std::string _direction_type;
    const bool is_frequency;
    const bool wheelchair_boarding;
    std::string _name;
    std::string _headsign;
    std::string _meta_vj_name;
    std::string _physical_mode;
    const uint32_t start_time;
    const uint32_t end_time;
    const uint32_t headway_secs;
    const nt::RTLevel vj_type;
    std::vector<ST> stop_times;
    nt::VehicleJourney* vj = nullptr;
    nt::ValidityPattern _vp;
    bool _bike_accepted = true;

    /// Construit un nouveau vehicle journey
    VJ(builder& b,
       std::string network_name,
       std::string line_name,
       const std::string& validity_pattern,
       std::string block_id,
       const bool is_frequency,
       const bool wheelchair_boarding = true,
       std::string name = "",
       std::string headsign = "",
       std::string meta_vj_name = "",
       std::string physical_mode = "",
       const uint32_t start_time = 0,
       const uint32_t end_time = 0,
       const uint32_t headway_secs = 0,
       const nt::RTLevel vj_type = nt::RTLevel::Base);

    VJ(VJ&&) = default;
    VJ& operator=(VJ&&) = delete;
    VJ(const VJ&) = delete;
    VJ& operator=(const VJ&) = delete;
    ~VJ() { make(); }  // The destructor create the vj as we need the stop times to create it.

    /// Ajout un nouveau stopTime
    /// Lorsque le depart n'est pas specifié, on suppose que c'est le même qu'à l'arrivée
    /// Si le stopPoint n'est pas connu, on le crée avec un stopArea ayant le même nom
    VJ& operator()(const std::string& sp_name,
                   int arrival,
                   int departure = -1,
                   uint16_t local_trafic_zone = std::numeric_limits<uint16_t>::max(),
                   bool drop_off_allowed = true,
                   bool pick_up_allowed = true,
                   int alighting_duration = 0,
                   int boarding_duration = 0,
                   bool skipped_stop = false);

    VJ& operator()(const std::string& stopPoint,
                   const std::string& arrival,
                   const std::string& departure = "",
                   uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max(),
                   bool drop_off_allowed = true,
                   bool pick_up_allowed = true,
                   int alighting_duration = 0,
                   int boarding_duration = 0,
                   bool skipped_stop = false);

    VJ& block_id(const std::string& b) {
        _block_id = b;
        return *this;
    }

    VJ& route(const std::string& r, const std::string d = "") {
        _route_name = r;
        _direction_type = d;
        return *this;
    }
    VJ& bike_accepted(bool b) {
        _bike_accepted = b;
        return *this;
    }

    // set the shape to the last stop point
    VJ& st_shape(const navitia::type::LineString& shape);

    VJ& name(const std::string& u) {
        _name = u;
        return *this;
    }
    VJ& headsign(const std::string& u) {
        _headsign = u;
        return *this;
    }
    VJ& valid_all_days() {
        _vp.days.set();
        return *this;
    }
    VJ& meta_vj(const std::string& m) {
        _meta_vj_name = m;
        return *this;
    }
    VJ& vp(const std::string& v) {
        _vp = {_vp.beginning_date, v};
        return *this;
    }
    VJ& physical_mode(const std::string& p) {
        _physical_mode = p;
        return *this;
    }

    // create the vj
    nt::VehicleJourney* make();
};

struct SP {
    using StopPointCodes = std::map<std::string, std::vector<std::string>>;

    static navitia::type::StopPoint* create_stop_point(builder& b,
                                                       navitia::type::StopArea* sa,
                                                       const std::string& name,
                                                       const StopPointCodes& codes = {},
                                                       double x = 0,
                                                       double y = 0,
                                                       bool wheelchair_boarding = true,
                                                       bool bike_accepted = true);
};

struct SA {
    builder& b;
    navitia::type::StopArea* sa;

    /// Create a new stopArea
    SA(builder& b,
       const std::string& sa_name,
       double x,
       double y,
       bool create_sp = true,
       bool wheelchair_boarding = true,
       bool bike_accepted = true);

    /// Create a stopPoint in the current stopArea
    SA& operator()(const std::string& sp_name,
                   double x = 0,
                   double y = 0,
                   bool wheelchair_boarding = true,
                   bool bike_accepted = true);

    SA& operator()(const std::string& sp_name, const SP::StopPointCodes& codes);
};

struct DisruptionCreator;

struct Impacter {
    Impacter(builder&, nt::disruption::Disruption&);
    builder& b;
    boost::shared_ptr<nt::disruption::Impact> impact;
    const nt::disruption::Disruption& get_disruption() const;
    Impacter& uri(const std::string& u);
    Impacter& application_periods(const boost::posix_time::time_period& p);
    Impacter& application_patterns(const nt::disruption::ApplicationPattern& application_pattern);
    Impacter& severity(nt::disruption::Effect,
                       std::string uri = "",
                       const std::string& wording = "",
                       const std::string& color = "#FFFF00",
                       int priority = 0);

    Impacter& severity(const std::string& uri);                                   // link to existing severity
    Impacter& on(nt::Type_e type, const std::string& uri, nt::PT_Data& pt_data);  // add elt in informed_entities
    Impacter& on_line_section(const std::string& line_uri,
                              const std::string& start_stop_uri,
                              const std::string& end_stop_uri,
                              const std::vector<std::string>& route_uris,
                              nt::PT_Data& pt_data);  // add section in informed_entities
    Impacter& on_rail_section(const std::string& line_uri,
                              const std::string& start_stop_uri,
                              const std::string& end_stop_uri,
                              const std::vector<std::pair<std::string, uint32_t>>& blocked_stop_areas,
                              const std::vector<std::string>& route_uris,
                              nt::PT_Data& pt_data);
    Impacter& msg(nt::disruption::Message);
    Impacter& msg(const std::string& text, nt::disruption::ChannelType = nt::disruption::ChannelType::email);
    Impacter& publish(const boost::posix_time::time_period& p);
};

struct DisruptionCreator {
    builder& b;
    DisruptionCreator(builder&, const std::string& uri, nt::RTLevel lvl);

    Impacter& impact();
    DisruptionCreator& publication_period(const boost::posix_time::time_period& p) {
        disruption.publication_period = p;
        return *this;
    }
    DisruptionCreator& contributor(const std::string& c) {
        disruption.contributor = c;
        return *this;
    }
    DisruptionCreator& tag(const std::string& t);
    DisruptionCreator& tag_if_not_empty(const std::string& t);

    DisruptionCreator& properties(const std::vector<nt::disruption::Property>& properties);

    nt::disruption::Disruption& disruption;
    std::vector<Impacter> impacters;
};

struct builder {
    std::map<std::string, navitia::type::Line*> lines;
    std::map<std::string, navitia::type::StopArea*> sas;
    std::map<std::string, navitia::type::StopPoint*> sps;
    std::multimap<std::string, navitia::type::VehicleJourney*> block_vjs;

    boost::gregorian::date begin;
    const navitia::type::TimeZoneHandler* tz_handler;

    std::unique_ptr<navitia::type::Data> data = std::make_unique<navitia::type::Data>();
    navitia::georef::GeoRef street_network;

    // lazy init of vertexes used when creating ways with add_way
    boost::optional<navitia::georef::vertex_t> vertex_a;
    boost::optional<navitia::georef::vertex_t> vertex_b;

    static void make_builder(builder&) {}

    /// 'date' is the beggining date of all the validity patterns
    builder(const std::string& date,
            std::function<void(builder&)> builder_callback = make_builder,
            bool no_dummy = false,
            const std::string& publisher_name = "canal tp",
            const std::string& timezone_name = "UTC",
            navitia::type::TimeZoneHandler::dst_periods timezone = {});

    /// Create a discrete vehicle journey (no frequency, explicit stop times)
    VJ vj(const std::string& line_name,
          const std::string& validity_pattern = "11111111",
          const std::string& block_id = "",
          const bool wheelchair_boarding = true,
          const std::string& name = "",
          const std::string& headsign = "",
          const std::string& meta_vj = "",
          const std::string& physical_mode = "",
          const nt::RTLevel vj_type = nt::RTLevel::Base);

    VJ vj_with_network(const std::string& network_name,
                       const std::string& line_name,
                       const std::string& validity_pattern = "11111111",
                       const std::string& block_id = "",
                       const bool wheelchair_boarding = true,
                       const std::string& name = "",
                       const std::string& headsign = "",
                       const std::string& meta_vj = "",
                       const std::string& physical_mode = "",
                       const bool is_frequency = false,
                       const uint32_t start_time = 0,
                       const uint32_t end_time = 0,
                       const uint32_t headway_secs = 0,
                       const nt::RTLevel vj_type = nt::RTLevel::Base);

    VJ frequency_vj(const std::string& line_name,
                    const uint32_t start_time,
                    const uint32_t end_time,
                    const uint32_t headway_secs,
                    const std::string& network_name = "default_network",
                    const std::string& validity_pattern = "11111111",
                    const std::string& block_id = "",
                    const bool wheelchair_boarding = true,
                    const std::string& name = "",
                    const std::string& headsign = "",
                    const std::string& meta_vj = "");

    // Create a new stop area
    SA sa(const std::string& name,
          double x = 0,
          double y = 0,
          const bool create_sp = true,
          const bool wheelchair_boarding = true,
          const bool bike_accepted = true);
    SA sa(const std::string& name,
          navitia::type::GeographicalCoord geo,
          const bool create_sp = true,
          bool wheelchair_boarding = true) {
        return sa(name, geo.lon(), geo.lat(), create_sp, wheelchair_boarding);
    }

    /**
     * Create and add an object (the object must have an idx, a uri and a name)
     */
    template <typename T>
    T* add(const std::string& uri, const std::string& name) {
        T* obj = new T();
        auto& collection = data->get_data<T>();
        obj->idx = collection.size();
        obj->uri = uri;
        obj->name = name;
        collection.push_back(obj);
        return obj;
    }

    void add_access_point(const std::string& stop_point,
                          const std::string& access_point_name,
                          const bool is_entrance = true,
                          const bool is_exit = true,
                          const int length = 0,
                          const int traversal_time = 0,
                          const double lat = 0,
                          const double lon = 0,
                          const std::string& stop_code = "");

    void add_ticket(const std::string& ticket_id,
                    const std::string& line,
                    const int cost = 100,
                    const std::string& comment = "",
                    const std::string& currency = "euro");

    template <typename T>
    T* get(const std::string& uri) const {
        const auto& collection = data->get_assoc_data<T>();
        return find_or_default(uri, collection);
    }

    DisruptionCreator disrupt(nt::RTLevel lvl, const std::string& uri);
    Impacter impact(nt::RTLevel lvl, std::string disruption_uri = "");

    /// Make a connection
    void connection(const std::string& name1, const std::string& name2, float length);

    void build_blocks();
    void finish();
    void generate_dummy_basis();
    void manage_admin();
    void build_autocomplete();
    void fill_missing_destinations();

    void make();  // Build the all thing !
    void finalize_disruption_batch();

    navitia::georef::Way* add_way(const std::string& name, const std::string& way_type, const bool visible = true);

    const navitia::type::Data& get_data() { return *data.get(); }

    const navitia::type::MetaVehicleJourney* get_meta_vj(const std::string& meta_vj_uri) const {
        return data->pt_data->meta_vjs.get_mut(meta_vj_uri);
    }
};

}  // namespace ed
