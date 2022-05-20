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
#include "type/datetime.h"
#include "type/type_interfaces.h"
#include "type/connection.h"
#include "type/validity_pattern.h"
#include "type/geographical_coord.h"
#include "type/vehicle_journey.h"
#include "type/calendar.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/optional.hpp>

#include <climits>
#include <bitset>
#include <unordered_map>
#include <utility>

namespace nt = navitia::type;
using nt::idx_t;

namespace ed {
namespace types {

struct pt_object_header {
    pt_object_header(const nt::Header* h, nt::Type_e t) : pt_object(h), type(t) {}
    pt_object_header() = default;
    const nt::Header* pt_object = nullptr;
    nt::Type_e type = nt::Type_e::Unknown;
    bool operator<(const pt_object_header& other) const { return pt_object < other.pt_object; }
    bool operator==(const pt_object_header& other) const { return pt_object == other.pt_object; }
};

inline std::ostream& operator<<(std::ostream& os, const pt_object_header pt) {
    return os << pt.pt_object->uri;
}

// On importe quelques éléments de Navitia::type pour éviter les redondances
using nt::hasProperties;
using nt::hasVehicleProperties;
using nt::Header;
using nt::Nameable;

using nt::ValidityPattern;

struct Line;
struct LineGroup;
struct VehicleJourney;
struct StopPoint;
struct StopArea;
struct Network;
struct PhysicalMode;
struct CommercialMode;
struct Company;
struct Route;
struct Contributor;
struct Calendar;

struct StopTime;

using nt::ConnectionType;
using nt::VehicleJourneyType;
struct StopPointConnection : public Header, hasProperties {
    const static nt::Type_e type = nt::Type_e::Connection;

    StopPoint* departure;
    StopPoint* destination;
    int display_duration;
    int duration;
    int max_duration;
    ConnectionType connection_kind;

    StopPointConnection()
        : departure(nullptr),
          destination(nullptr),
          display_duration(0),
          duration(0),
          max_duration(0),
          connection_kind(ConnectionType::Default) {}

    bool operator<(const StopPointConnection& other) const;
};

struct Calendar : public Nameable, public Header {
    const static nt::Type_e type = nt::Type_e::Calendar;
    using Week = std::bitset<7>;
    Week week_pattern;
    std::vector<Line*> line_list;
    std::vector<boost::gregorian::date_period> period_list;
    std::vector<navitia::type::ExceptionDate> exceptions;

    ValidityPattern validity_pattern;  // computed validity pattern

    Calendar() = default;
    Calendar(boost::gregorian::date beginning_date);

    // we limit the validity pattern to the production period
    void build_validity_pattern(boost::gregorian::date_period production_period);

    bool operator<(const Calendar& other) const { return this < &other; }
};

struct AssociatedCalendar : public Header {
    /// calendar matched
    const Calendar* calendar;

    /// exceptions to this association (not to be mixed up with the exceptions in the calendar)
    /// the calendar exceptions change it's validity pattern
    /// the AssociatedCalendar exceptions are the differences between the vj validity pattern and the calendar's
    std::vector<navitia::type::ExceptionDate> exceptions;
};
struct MetaVehicleJourney;

struct StopArea : public Header, Nameable, hasProperties {
    const static nt::Type_e type = nt::Type_e::StopArea;
    nt::GeographicalCoord coord;
    std::pair<std::string, boost::local_time::time_zone_ptr> time_zone_with_name;

    StopArea() = default;
    StopArea(idx_t idx, std::string uri) : Header(idx, uri), Nameable(uri) {}

    bool operator<(const StopArea& other) const;
};

struct Contributor : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Contributor;
    std::string website;
    std::string license;
    Contributor() = default;

    bool operator<(const Contributor& other) const { return this->name < other.name; }
};

struct AccessPoint : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::AccessPoint;
    nt::GeographicalCoord coord;

    std::string parent_station;
    std::string stop_code;

    AccessPoint() = default;
    AccessPoint(int idx, std::string uri) : Header(idx, uri), Nameable(uri) {}

    bool operator<(const AccessPoint& other) const;
    bool operator!=(const AccessPoint& sp) const;
};

struct Dataset : public Header {
    const static nt::Type_e type = nt::Type_e::Dataset;
    Contributor* contributor = nullptr;
    boost::gregorian::date_period validation_period{boost::gregorian::date(), boost::gregorian::date()};
    navitia::type::RTLevel realtime_level = navitia::type::RTLevel::Base;
    std::string desc;
    // Contributor data types in fusio example : Google, OBiTi, ChouetteV2 etc..
    std::string system;
    bool operator<(const Dataset& other) const { return this->idx < other.idx; }
};

struct Network : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Network;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;
    int sort = std::numeric_limits<int>::max();

    bool operator<(const Network& other) const { return this->name < other.name; }
};

struct PathWay : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::PathWay;
    std::string from_stop_id;
    std::string to_stop_id;
    int pathway_mode;
    bool is_bidirectional;
    int length;
    int traversal_time;
    int stair_count;
    int max_slope;
    int min_width;
    std::string signposted_as;
    std::string reversed_signposted_as;

    bool operator<(const Network& other) const { return this->name < other.name; }
};

struct Company : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Company;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    bool operator<(const Company& other) const { return this->name < other.name; }
};

struct CommercialMode : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::CommercialMode;

    bool operator<(const CommercialMode& other) const;
};

struct PhysicalMode : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::PhysicalMode;
    boost::optional<double> co2_emission;
    PhysicalMode() = default;

    bool operator<(const PhysicalMode& other) const;
};

struct Line : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    std::string text_color;
    int sort = std::numeric_limits<int>::max();

    CommercialMode* commercial_mode = nullptr;
    Network* network = nullptr;
    Company* company = nullptr;
    nt::MultiLineString shape;
    boost::optional<boost::posix_time::time_duration> opening_time, closing_time;

    bool operator<(const Line& other) const;
};

struct LineGroup : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::LineGroup;
    Line* main_line;

    bool operator<(const LineGroup& other) const;
};

struct LineGroupLink {
    LineGroup* line_group;
    Line* line;
};

struct Route : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Route;
    Line* line;
    nt::MultiLineString shape;
    StopArea* destination = nullptr;
    std::string direction_type;

    bool operator<(const Route& other) const;
};

struct VehicleJourney : public Header, Nameable, hasVehicleProperties {
    const static nt::Type_e type = nt::Type_e::VehicleJourney;
    Route* route = nullptr;
    Company* company = nullptr;
    PhysicalMode* physical_mode = nullptr;
    Dataset* dataset = nullptr;
    // Vehicle* vehicle;
    bool wheelchair_boarding = false;
    navitia::type::VehicleJourneyType vehicle_journey_type = navitia::type::VehicleJourneyType::regular;
    ValidityPattern* validity_pattern = nullptr;
    std::vector<StopTime*> stop_time_list;  // May be empty
    StopTime* first_stop_time = nullptr;
    std::string block_id;
    std::string odt_message;
    std::string meta_vj_name;  // link to it's meta vj
    std::string shape_id;
    std::string headsign;

    int start_time = std::numeric_limits<int>::max();    /// First departure of vehicle
    int end_time = std::numeric_limits<int>::max();      /// Last departure of vehicle journey
    int headway_secs = std::numeric_limits<int>::max();  /// Seconds between each departure.
    bool is_frequency() const { return start_time != std::numeric_limits<int>::max(); }

    bool is_adapted;
    ValidityPattern* adapted_validity_pattern = nullptr;
    std::vector<VehicleJourney*> adapted_vehicle_journey_list;
    VehicleJourney* theoric_vehicle_journey = nullptr;
    VehicleJourney* prev_vj = nullptr;  // previous vj with the same block_id
    VehicleJourney* next_vj = nullptr;  // next vj with the same block_id

    navitia::type::RTLevel realtime_level = navitia::type::RTLevel::Base;

    VehicleJourney(int idx = 0, std::string uri = "", ValidityPattern* vp = nullptr)
        : Header(idx, uri), Nameable(uri), validity_pattern(vp) {}

    // Return the smallest time within its stop_times
    int earliest_time() const;

    bool operator<(const VehicleJourney& other) const;
    bool joins_on_different_stop_points(const ed::types::VehicleJourney& prev_vj) const;
    bool starts_with_stayin_on_same_stop_point() const;
    bool ends_with_stayin_on_same_stop_point() const;
};

struct StopPoint : public Header, Nameable, hasProperties {
    const static nt::Type_e type = nt::Type_e::StopPoint;
    nt::GeographicalCoord coord;
    std::string fare_zone;
    std::string address_id = "";
    bool is_zonal = false;
    boost::optional<nt::MultiPolygon> area;

    std::string platform_code;

    StopArea* stop_area;
    Network* network;

    StopPoint() : fare_zone(), stop_area(nullptr), network(nullptr) {}
    StopPoint(int idx, std::string uri, StopArea* sa = nullptr, Network* n = nullptr)
        : Header(idx, uri), Nameable(uri), stop_area(sa), network(n) {}

    bool operator<(const StopPoint& other) const;
    bool operator!=(const StopPoint& sp) const;
};

struct Shape {
    size_t idx = 0;
    nt::LineString geom;
    Shape(nt::LineString ls) : geom(std::move(ls)) {}
};

struct StopTime {
    size_t idx = 0;
    int arrival_time = 0;    /// Number of seconds from midnight can be negative when
    int departure_time = 0;  /// we shift in UTC conversion
    int boarding_time = 0;   /// Time at which the user should be to start the boarding process
    int alighting_time = 0;  /// Time at which the user will have finish the alighting process
    VehicleJourney* vehicle_journey = nullptr;
    StopPoint* stop_point = nullptr;
    std::shared_ptr<Shape> shape_from_prev;
    unsigned int order = 0;
    bool ODT = false;
    bool pick_up_allowed = false;
    bool drop_off_allowed = false;
    bool skipped_stop = false;
    bool is_frequency = false;
    bool wheelchair_boarding = false;
    bool date_time_estimated = false;
    std::string headsign;

    uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max();

    StopTime() = default;
    StopTime(idx_t idx,
             int arr,
             int dep,
             int board,
             int ali,
             VehicleJourney* vj,
             StopPoint* sp,
             std::shared_ptr<Shape> shape,
             uint order,
             bool odt,
             bool pick,
             bool drop,
             bool freq,
             bool wheel,
             bool estim,
             bool skipped,
             std::string&& headsign)
        : idx(idx),
          arrival_time(arr),
          departure_time(dep),
          boarding_time(board),
          alighting_time(ali),
          vehicle_journey(vj),
          stop_point(sp),
          shape_from_prev(std::move(shape)),
          order(order),
          ODT(odt),
          pick_up_allowed(pick),
          drop_off_allowed(drop),
          is_frequency(freq),
          wheelchair_boarding(wheel),
          date_time_estimated(estim),
          skipped_stop(skipped),
          headsign(std::move(headsign)) {}
    bool operator<(const StopTime& other) const;
    void shift_times(int n_days) {
        arrival_time += n_days * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
        departure_time += n_days * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
        alighting_time += n_days * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
        boarding_time += n_days * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
        assert(arrival_time >= 0 && departure_time >= 0 && alighting_time >= 0 && boarding_time >= 0);
    }
};

/**
 * A meta vj is a shell around some vehicle journeys
 *
 * It has 2 purposes:
 *
 *  - to store the adapted and real time vj
 *
 *  - sometime we have to split a vj.
 *    For example we have to split a vj because of dst (day saving light see gtfs parser for that)
 *    the meta vj can thus make the link between the split vjs
 *    *NOTE*: An IMPORTANT prerequisite is that ALL theoric vj have the same local time
 *            (even if the UTC time is different because of DST)
 *            That prerequisite is very important for calendar association and departure board over period
 *
 *
 */
struct MetaVehicleJourney : public Header {
    // TODO if needed use a flat_enum_map
    std::vector<VehicleJourney*> theoric_vj;
    std::vector<VehicleJourney*> adapted_vj;
    std::vector<VehicleJourney*> real_time_vj;

    /// map of the calendars that nearly match union of the validity pattern
    /// of the theoric vj, key is the calendar name
    std::map<std::string, AssociatedCalendar*> associated_calendars;
};

/// Données Geographique

struct Node {
    size_t id;
    bool is_used;
    navitia::type::GeographicalCoord coord;
    Node() : id(0), is_used(false) {}
};

struct Admin {
    size_t id;
    bool is_used;
    std::string level;
    std::string insee;
    std::string name;
    std::string postcode;
    navitia::type::GeographicalCoord coord;
    std::set<std::string> postal_codes;
    Admin() : id(0), is_used(false), level("8") {}
};
struct Edge;
struct Way {
    size_t id;
    std::string uri;
    bool is_used;
    Admin* admin;
    std::string name;
    std::string type;
    bool visible;
    std::vector<Edge*> edges;
    Way() : id(0), uri(""), is_used(true), admin(nullptr), name(""), type(""), visible(true) {}
};

struct Edge {
    Way* way;
    Node* source;
    Node* target;
    int length;
    Edge() : way(nullptr), source(nullptr), target(nullptr), length(0) {}
};

struct HouseNumber {
    std::string number;
    Way* way;
    navitia::type::GeographicalCoord coord;

    HouseNumber() : number(""), way(nullptr) {}
};

struct PoiType {
    size_t id = 0;
    std::string name = "";

    PoiType() = default;
    PoiType(size_t id, std::string name) : id(id), name(std::move(name)) {}
};

struct Poi {
    size_t id = 0;
    uint64_t osm_id = std::numeric_limits<uint64_t>::max();
    std::string name = "";
    int weight = 0;
    bool visible = true;
    PoiType* poi_type = nullptr;
    navitia::type::GeographicalCoord coord;
    std::map<std::string, std::string> properties;
    std::string address_number = "";
    std::string address_name = "";
    std::string uri = "poi:";

    Poi() = default;
    Poi(const std::string& uri_id) { uri += uri_id; }
};

/**
 * A AdminStopArea is a relation between an admin code and a list of stop area.
 *
 * Beware that a stop_area can be shared between different AdminStopArea
 */
struct AdminStopArea {
    std::string admin;
    std::vector<const StopArea*> stop_area;
};

#define ASSOCIATE_ED_TYPE(type_name, collection_name)                                                    \
    inline nt::Type_e get_associated_enum(const ed::types::type_name&) { return nt::Type_e::type_name; } \
    inline nt::Type_e get_associated_enum(const ed::types::type_name*) { return nt::Type_e::type_name; }

ITERATE_NAVITIA_PT_TYPES(ASSOCIATE_ED_TYPE)
inline nt::Type_e get_associated_enum(const ed::types::StopTime&) {
    return nt::Type_e::StopTime;
}
inline nt::Type_e get_associated_enum(const ed::types::StopTime*) {
    return nt::Type_e::StopTime;
}
inline nt::Type_e get_associated_enum(const ed::types::AccessPoint&) {
    return nt::Type_e::AccessPoint;
}
inline nt::Type_e get_associated_enum(const ed::types::AccessPoint*) {
    return nt::Type_e::AccessPoint;
}

template <typename T>
pt_object_header make_pt_object(const T& h) {
    return pt_object_header(h, get_associated_enum(h));
}
}  // namespace types
}  // namespace ed
