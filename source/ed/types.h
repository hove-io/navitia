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
#include <boost/date_time/gregorian/gregorian.hpp>
#include <bitset>
#include <unordered_map>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include "type/datetime.h"

#include "type/type.h"
#include "limits.h"


namespace nt = navitia::type;
using nt::idx_t;

namespace ed{ namespace types{

struct pt_object_header {
    pt_object_header (const nt::Header* h, nt::Type_e t): pt_object(h), type(t) {}
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
using nt::Nameable;
using nt::Header;
using nt::hasProperties;
using nt::hasVehicleProperties;


#define FORWARD_CLASS_DECLARE(type_name, collection_name) struct type_name;
ITERATE_NAVITIA_PT_TYPES(FORWARD_CLASS_DECLARE)
struct StopTime;

using nt::ConnectionType;
using nt::VehicleJourneyType;
struct StopPointConnection: public Header, hasProperties {
    const static nt::Type_e type = nt::Type_e::Connection;

    StopPoint* departure;
    StopPoint* destination;
    int display_duration;
    int duration;
    int max_duration;
    ConnectionType connection_kind;

    StopPointConnection() : departure(NULL), destination(NULL), display_duration(0), duration(0),
        max_duration(0), connection_kind(ConnectionType::Default){}

   bool operator<(const StopPointConnection& other) const;

};

struct ValidityPattern: public Header {
    const static nt::Type_e type = nt::Type_e::ValidityPattern;
private:
    bool is_valid(int duration);
    int slide(boost::gregorian::date day) const;
public:
    using year_bitset = std::bitset<366>;
    year_bitset days;
    boost::gregorian::date beginning_date;
    ValidityPattern(){}
    ValidityPattern(boost::gregorian::date beginning_date, const std::string & vp = "") : days(vp), beginning_date(beginning_date){}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);

    bool check(int day) const;
    bool check(boost::gregorian::date day) const;

    bool operator<(const ValidityPattern& other) const;
    bool operator==(const ValidityPattern& other) const;
};

struct Calendar : public Nameable, public Header {
    const static nt::Type_e type = nt::Type_e::Calendar;
    typedef std::bitset<7> Week;
    Week week_pattern;
    std::vector<Line*> line_list;
    std::vector<boost::gregorian::date_period> period_list;
    std::vector<navitia::type::ExceptionDate> exceptions;

    ValidityPattern validity_pattern; //computed validity pattern

    Calendar() {}
    Calendar(boost::gregorian::date beginning_date);

    //we limit the validity pattern to the production period
    void build_validity_pattern(boost::gregorian::date_period production_period);

    bool operator<(const Calendar & other) const { return this < &other; }
};

struct AssociatedCalendar : public Header {
    ///calendar matched
    const Calendar* calendar;

    ///exceptions to this association (not to be mixed up with the exceptions in the calendar)
    ///the calendar exceptions change it's validity pattern
    /// the AssociatedCalendar exceptions are the differences between the vj validity pattern and the calendar's
    std::vector<navitia::type::ExceptionDate> exceptions;
};
struct MetaVehicleJourney;

struct StopArea : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopArea;
    nt::GeographicalCoord coord;
    std::pair<std::string, boost::local_time::time_zone_ptr> time_zone_with_name;

    StopArea() {}

    bool operator<(const StopArea& other) const;
};


struct Contributor : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Contributor;
    Contributor() {}

    bool operator<(const Contributor& other)const{ return this->name < other.name;}
};

struct Network : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Network;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;
    int sort = std::numeric_limits<int>::max();


    bool operator<(const Network& other)const{ return this->name < other.name;}
};

struct Company : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Company;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    bool operator<(const Company& other)const{ return this->name < other.name;}
};

struct CommercialMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::CommercialMode;

    bool operator<(const CommercialMode& other)const ;
};

struct PhysicalMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::PhysicalMode;
    boost::optional<double> co2_emission;
    PhysicalMode(){}

    bool operator<(const PhysicalMode& other) const;
};

struct Line : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort = std::numeric_limits<int>::max();

    CommercialMode* commercial_mode = nullptr;
    Network* network = nullptr;
    Company* company = nullptr;
    nt::MultiLineString shape;
    boost::optional<boost::posix_time::time_duration> opening_time,
                                                      closing_time;

    bool operator<(const Line & other) const;

};

struct LineGroup : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::LineGroup;
    std::string name;
    Line* main_line;

    bool operator<(const LineGroup & other) const;
};

struct LineGroupLink {
    LineGroup* line_group;
    Line* line;
};

struct Route : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Route;
    Line * line;
    nt::MultiLineString shape;
    StopArea* destination = nullptr;

    bool operator<(const Route& other) const;
};

struct JourneyPattern : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPattern;
    bool is_frequence;
    Route* route;
    PhysicalMode* physical_mode;
    std::vector<JourneyPatternPoint*> journey_pattern_point_list;
    nt::LineString shape;

    JourneyPattern(): is_frequence(false), route(NULL), physical_mode(NULL){}

    bool operator<(const JourneyPattern& other) const;
 };

struct VehicleJourney: public Header, Nameable, hasVehicleProperties{
    const static nt::Type_e type = nt::Type_e::VehicleJourney;
    JourneyPattern* journey_pattern = nullptr;
    Company* company = nullptr;
    PhysicalMode* physical_mode = nullptr; // Read in journey_pattern
    Line * tmp_line = nullptr; // May be empty
    Route* tmp_route = nullptr;
    //Vehicle* vehicle;
    bool wheelchair_boarding = false;
    navitia::type::VehicleJourneyType vehicle_journey_type = navitia::type::VehicleJourneyType::regular;
    ValidityPattern* validity_pattern = nullptr;
    std::vector<StopTime*> stop_time_list; // May be empty
    StopTime * first_stop_time = nullptr;
    std::string block_id;
    std::string odt_message;
    std::string meta_vj_name; //link to it's meta vj
    std::string shape_id;
    int16_t utc_to_local_offset = 0; // shift used to convert the local time from the data to utc, in seconds

    int start_time = std::numeric_limits<int>::max(); /// First departure of vehicle
    int end_time = std::numeric_limits<int>::max(); /// Last departure of vehicle journey
    int headway_secs = std::numeric_limits<int>::max(); /// Seconds between each departure.
    bool is_frequency() const {return start_time != std::numeric_limits<int>::max();}

    bool is_adapted;
    ValidityPattern* adapted_validity_pattern = nullptr;
    std::vector<VehicleJourney*> adapted_vehicle_journey_list;
    VehicleJourney* theoric_vehicle_journey = nullptr;
    VehicleJourney* prev_vj = nullptr;
    VehicleJourney* next_vj = nullptr;

    bool operator<(const VehicleJourney& other) const;
};


struct JourneyPatternPoint : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPatternPoint;
    int order;
    bool main_stop_point;
    int fare_section;
    JourneyPattern* journey_pattern;
    StopPoint* stop_point;
    nt::LineString shape_from_prev;

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern(NULL), stop_point(NULL){}

    bool operator<(const JourneyPatternPoint& other) const;
};

struct StopPoint : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopPoint;
    nt::GeographicalCoord coord;
    int fare_zone;
    bool is_zonal = false;
    boost::optional<nt::MultiPolygon> area;

    std::string platform_code;

    StopArea* stop_area;
    Network* network;

    StopPoint(): fare_zone(0), stop_area(NULL), network(NULL) {}

    bool operator<(const StopPoint& other) const;
};

struct StopTime {
    size_t idx;
    int arrival_time; /// Number of seconds from midnight can be negative when
    int departure_time; /// we shift in UTC conversion
    VehicleJourney* vehicle_journey;
    JourneyPatternPoint* journey_pattern_point;
    StopPoint* tmp_stop_point;// not mandatory
    unsigned int order;
    bool ODT;
    bool pick_up_allowed;
    bool drop_off_allowed;
    bool is_frequency;
    bool wheelchair_boarding;
    bool date_time_estimated;

    uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max();

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey(NULL), journey_pattern_point(NULL), tmp_stop_point(NULL), order(0),
        ODT(false), pick_up_allowed(false), drop_off_allowed(false), is_frequency(false), wheelchair_boarding(false),date_time_estimated(false) {}

    bool operator<(const StopTime& other) const;
    void shift_times(int n_days) {
        arrival_time += n_days * navitia::DateTimeUtils::SECONDS_PER_DAY;
        departure_time += n_days * navitia::DateTimeUtils::SECONDS_PER_DAY;
        assert(arrival_time >= 0 && departure_time >= 0);
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
struct MetaVehicleJourney {
    //store the name ?
    //TODO if needed use a flat_enum_map
    std::vector<VehicleJourney*> theoric_vj;
    std::vector<VehicleJourney*> adapted_vj;
    std::vector<VehicleJourney*> real_time_vj;

    /// map of the calendars that nearly match union of the validity pattern
    /// of the theoric vj, key is the calendar name
    std::map<std::string, AssociatedCalendar*> associated_calendars;
};

/// Données Geographique

struct Node{
    size_t id;
    bool is_used;
    navitia::type::GeographicalCoord coord;
    Node():id(0), is_used(false){}
};

struct Admin{
    size_t id;
    bool is_used;
    std::string level;
    std::string insee;
    std::string name;
    std::string postcode;
    navitia::type::GeographicalCoord coord;
    std::set<std::string> postal_codes;
    Admin():id(0), is_used(false), level("8"){}
};
struct Edge;
struct Way{
    size_t id;
    std::string uri;
    bool is_used;
    Admin* admin;
    std::string name;
    std::string type;
    std::vector<Edge*> edges;
    Way():id(0), uri(""), is_used(true), admin(nullptr), name(""), type(""){}
};

struct Edge{
    Way* way;
    Node* source;
    Node* target;
    int length;
    Edge():way(nullptr), source(nullptr), target(nullptr), length(0){}
};

struct HouseNumber{
    std::string number;
    Way* way;
    navitia::type::GeographicalCoord coord;

    HouseNumber(): number(""), way(nullptr){}

};

struct PoiType {
    size_t id = 0;
    std::string name = "";
    
    PoiType() {}
    PoiType(size_t id, const std::string& name): id(id), name(name){}
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

    Poi() {}
};

/**
 * A AdminStopArea is a relation between an admin code and a list of stop area.
 *
 * Beware that a stop_area can be shared between different AdminStopArea
 */
struct AdminStopArea{
    std::string admin;
    std::vector<const StopArea*> stop_area;
};

#define ASSOCIATE_ED_TYPE(type_name, collection_name) \
        inline nt::Type_e get_associated_enum(const ed::types::type_name&) { return nt::Type_e::type_name; } \
        inline nt::Type_e get_associated_enum(const ed::types::type_name*) { return nt::Type_e::type_name; }

ITERATE_NAVITIA_PT_TYPES(ASSOCIATE_ED_TYPE)
inline nt::Type_e get_associated_enum(const ed::types::StopTime&) { return nt::Type_e::StopTime; }
inline nt::Type_e get_associated_enum(const ed::types::StopTime*) { return nt::Type_e::StopTime; }


template <typename T>
pt_object_header make_pt_object(const T& h) {
    return pt_object_header(h, get_associated_enum(h));
}
}}//end namespace ed::types
