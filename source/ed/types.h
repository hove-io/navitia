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
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "type/type.h"

namespace nt = navitia::type;
using nt::idx_t;

namespace ed{ namespace types{

// On importe quelques éléments de Navitia::type pour éviter les redondances
using nt::Nameable;
using nt::Header;
using nt::hasProperties;
using nt::hasVehicleProperties;


#define FORWARD_CLASS_DECLARE(type_name, collection_name) class type_name;
ITERATE_NAVITIA_PT_TYPES(FORWARD_CLASS_DECLARE)
class StopTime;

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

    navitia::type::StopPointConnection* get_navitia_type() const;

    StopPointConnection() : departure(NULL), destination(NULL), display_duration(0),duration(0),
        max_duration(0), connection_kind(ConnectionType::Default){}

   bool operator<(const StopPointConnection& other) const;

};

struct Calendar : public Nameable, public Header {
    const static nt::Type_e type = nt::Type_e::Calendar;
    typedef std::bitset<7> Week;
    std::string external_code;
    Week week_pattern;
    std::vector<Line*> line_list;
    std::vector<boost::gregorian::date_period> period_list;
    std::vector<navitia::type::ExceptionDate> exceptions;

    navitia::type::Calendar* get_navitia_type() const;

    Calendar() {}

    bool operator<(const Calendar & other) const { return this < &other; }
};

struct StopArea : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopArea;
    std::string external_code;
    nt::GeographicalCoord coord;
    std::pair<std::string, boost::local_time::time_zone_ptr> time_zone_with_name;

    navitia::type::StopArea* get_navitia_type() const;

    StopArea() {}

    bool operator<(const StopArea& other) const;
};


struct Contributor : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Contributor;
    Contributor() {}

    nt::Contributor* get_navitia_type() const;

    bool operator<(const Contributor& other)const{ return this->name < other.name;}
};

struct Network : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Network;
    std::string external_code;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;
    int sort = std::numeric_limits<int>::max();


    navitia::type::Network* get_navitia_type() const;

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

    nt::Company* get_navitia_type() const;

    bool operator<(const Company& other)const{ return this->name < other.name;}
};

struct CommercialMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::CommercialMode;
    nt::CommercialMode* get_navitia_type() const;

    bool operator<(const CommercialMode& other)const ;
};

struct PhysicalMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::PhysicalMode;
    PhysicalMode() {}

    nt::PhysicalMode* get_navitia_type() const;

    bool operator<(const PhysicalMode& other) const;
};

struct Line : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Line;
    std::string external_code;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort = std::numeric_limits<int>::max();

    CommercialMode* commercial_mode;
    Network* network;
    Company* company;

    Line(): color(""), commercial_mode(NULL), network(NULL), company(NULL){}

    nt::Line* get_navitia_type() const;

    bool operator<(const Line & other) const;
};

struct Route : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Route;
    std::string external_code;
    Line * line;

    navitia::type::Route* get_navitia_type() const;

    bool operator<(const Route& other) const;
};

struct JourneyPattern : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPattern;
    bool is_frequence;
    Route* route;
    PhysicalMode* physical_mode;
    std::vector<JourneyPatternPoint*> journey_pattern_point_list;

    JourneyPattern(): is_frequence(false), route(NULL), physical_mode(NULL){};

    navitia::type::JourneyPattern* get_navitia_type() const;

    bool operator<(const JourneyPattern& other) const;
 };

struct VehicleJourney: public Header, Nameable, hasVehicleProperties{
    const static nt::Type_e type = nt::Type_e::VehicleJourney;
    std::string external_code;
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

    int start_time = std::numeric_limits<int>::max(); /// First departure of vehicle
    int end_time = std::numeric_limits<int>::max(); /// Last departure of vehicle journey
    int headway_secs = std::numeric_limits<int>::max(); /// Seconds between each departure.

    bool is_adapted;
    ValidityPattern* adapted_validity_pattern = nullptr;
    std::vector<VehicleJourney*> adapted_vehicle_journey_list;
    VehicleJourney* theoric_vehicle_journey = nullptr;
    VehicleJourney* prev_vj = nullptr;
    VehicleJourney* next_vj = nullptr;

    navitia::type::VehicleJourney* get_navitia_type() const;

    bool operator<(const VehicleJourney& other) const;
};


struct JourneyPatternPoint : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPatternPoint;
    int order;
    bool main_stop_point;
    int fare_section;
    JourneyPattern* journey_pattern;
    StopPoint* stop_point;

    nt::JourneyPatternPoint* get_navitia_type() const;

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern(NULL), stop_point(NULL){}

    bool operator<(const JourneyPatternPoint& other) const;
};

struct ValidityPattern: public Header {
    const static nt::Type_e type = nt::Type_e::ValidityPattern;
private:
    bool is_valid(int duration);
public:
    std::bitset<366> days;
    boost::gregorian::date beginning_date;
    ValidityPattern(){}
    ValidityPattern(boost::gregorian::date beginning_date, const std::string & vp = "") : days(vp), beginning_date(beginning_date){}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);

    bool check(int day) const;

    nt::ValidityPattern* get_navitia_type() const;

    bool operator<(const ValidityPattern& other) const;
    bool operator==(const ValidityPattern& other) const;
};

struct StopPoint : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopPoint;
    std::string external_code;
    nt::GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string platform_code;

    StopArea* stop_area;
    Network* network;

    StopPoint(): fare_zone(0), stop_area(NULL), network(NULL) {}

    nt::StopPoint* get_navitia_type() const;

    bool operator<(const StopPoint& other) const;
};

struct StopTime : public Nameable {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    VehicleJourney* vehicle_journey;
    JourneyPatternPoint* journey_pattern_point;
    StopPoint * tmp_stop_point;// ne pas remplir obligatoirement
    int order;
    bool ODT;
    bool pick_up_allowed;
    bool drop_off_allowed;
    bool is_frequency;
    bool wheelchair_boarding;
    bool date_time_estimated;

    uint16_t local_traffic_zone;

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey(NULL), journey_pattern_point(NULL), tmp_stop_point(NULL), order(0),
        ODT(false), pick_up_allowed(false), drop_off_allowed(false), is_frequency(false), wheelchair_boarding(false),date_time_estimated(false),
                local_traffic_zone(std::numeric_limits<uint16_t>::max()) {}

    navitia::type::StopTime* get_navitia_type() const;

    bool operator<(const StopTime& other) const;
};


struct MetaVehicleJourney {
    std::vector<VehicleJourney*> theoric_vj;
    std::vector<VehicleJourney*> adapted_vj;
    std::vector<VehicleJourney*> real_time_vj;
};

/// Données Geographique

struct Node{
    size_t id;
    bool is_used;
    navitia::type::GeographicalCoord coord;
    Node():id(0), is_used(false){};
};

struct Admin{
    size_t id;
    bool is_used;
    std::string level;
    std::string insee;
    std::string name;
    std::string postcode;
    navitia::type::GeographicalCoord coord;
    Admin():id(0), is_used(false), level("8"){};
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
    Way():id(0), uri(""), is_used(true), admin(nullptr), name(""), type(""){};
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

struct PoiType{
    size_t id;
    std::string name;

    PoiType(): id(0), name(""){}
    PoiType(size_t id, const std::string& name): id(id), name(name){}
};


struct Poi{
    size_t id;
    std::string name;
    int weight;
    bool visible;
    PoiType* poi_type;
    navitia::type::GeographicalCoord coord;
    std::map<std::string, std::string> properties;
    std::string address_number;
    std::string address_name;
    Poi(): id(0), name(""), weight(0), visible(true), poi_type(nullptr), address_number(""), address_name(""){}

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

}}//end namespace ed::types
