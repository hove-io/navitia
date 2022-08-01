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

#include "utils/exception.h"
#include "type/type_interfaces.h"
#include "type/fwd_type.h"
#include "type/stop_time.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/range/iterator_range.hpp>

#include <atomic>
#include <map>
#include <utility>

#include <vector>
#include <string>
#include <set>

using BlockedSAList = std::map<int, std::string>;
using ConcatenateBlockedSASequence = std::string;

namespace navitia {
namespace type {
namespace disruption {

enum class Effect {
    NO_SERVICE = 0,
    REDUCED_SERVICE,
    SIGNIFICANT_DELAYS,
    DETOUR,
    ADDITIONAL_SERVICE,
    MODIFIED_SERVICE,
    OTHER_EFFECT,
    UNKNOWN_EFFECT,
    STOP_MOVED
};

enum class ActiveStatus { past = 0, active = 1, future = 2 };

enum class ChannelType { web = 0, sms, email, mobile, notification, twitter, facebook, unknown_type, title, beacon };

inline std::string to_string(Effect effect) {
    switch (effect) {
        case Effect::NO_SERVICE:
            return "NO_SERVICE";
        case Effect::REDUCED_SERVICE:
            return "REDUCED_SERVICE";
        case Effect::SIGNIFICANT_DELAYS:
            return "SIGNIFICANT_DELAYS";
        case Effect::DETOUR:
            return "DETOUR";
        case Effect::ADDITIONAL_SERVICE:
            return "ADDITIONAL_SERVICE";
        case Effect::MODIFIED_SERVICE:
            return "MODIFIED_SERVICE";
        case Effect::OTHER_EFFECT:
            return "OTHER_EFFECT";
        case Effect::UNKNOWN_EFFECT:
            return "UNKNOWN_EFFECT";
        case Effect::STOP_MOVED:
            return "STOP_MOVED";
        default:
            throw navitia::exception("unhandled effect case");
    }
}

inline Effect from_string(const std::string& str) {
    if (str == "NO_SERVICE") {
        return Effect::NO_SERVICE;
    }
    if (str == "REDUCED_SERVICE") {
        return Effect::REDUCED_SERVICE;
    }
    if (str == "SIGNIFICANT_DELAYS") {
        return Effect::SIGNIFICANT_DELAYS;
    }
    if (str == "DETOUR") {
        return Effect::DETOUR;
    }
    if (str == "ADDITIONAL_SERVICE") {
        return Effect::ADDITIONAL_SERVICE;
    }
    if (str == "MODIFIED_SERVICE") {
        return Effect::MODIFIED_SERVICE;
    }
    if (str == "OTHER_EFFECT") {
        return Effect::OTHER_EFFECT;
    }
    if (str == "UNKNOWN_EFFECT") {
        return Effect::UNKNOWN_EFFECT;
    }
    if (str == "STOP_MOVED") {
        return Effect::STOP_MOVED;
    }
    throw navitia::exception("unhandled effect case");
}

inline std::string to_string(ChannelType ct) {
    switch (ct) {
        case ChannelType::web:
            return "web";
        case ChannelType::sms:
            return "sms";
        case ChannelType::email:
            return "email";
        case ChannelType::mobile:
            return "mobile";
        case ChannelType::notification:
            return "notification";
        case ChannelType::twitter:
            return "twitter";
        case ChannelType::facebook:
            return "facebook";
        case ChannelType::unknown_type:
            return "unknown_type";
        case ChannelType::title:
            return "title";
        case ChannelType::beacon:
            return "beacon";
        default:
            throw navitia::exception("unhandled channeltype case");
    }
}

struct Property {
    std::string key;
    std::string type;
    std::string value;

    bool operator<(const Property& right) const {
        if (key != right.key) {
            return key < right.key;
        }
        if (type != right.type) {
            return type < right.type;
        }
        return value < right.value;
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct Cause {
    std::string uri;
    std::string wording;
    std::string category;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct Severity {
    std::string uri;
    std::string wording;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;
    std::string color;

    int priority;

    Effect effect = Effect::UNKNOWN_EFFECT;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct UnknownPtObj {
    template <class Archive>
    void serialize(Archive&, const unsigned int) {}
};
struct LineSection {
    Line* line = nullptr;
    StopArea* start_point = nullptr;
    StopArea* end_point = nullptr;
    std::vector<Route*> routes;
    template <class archive>
    void serialize(archive& ar, const unsigned int);

    std::set<StopPoint*> get_stop_points_section() const;
};

struct RailSection {
    RailSection() = default;
    RailSection(StopArea* start_,
                StopArea* end_,
                const std::vector<StopArea*>& blockeds_,
                const std::vector<StopArea*>& impacted_stop_areas_,
                Line* line_,
                const std::vector<Route*>& routes_)
        : start(start_),
          end(end_),
          blockeds(blockeds_),
          impacted_stop_areas(impacted_stop_areas_),
          line(line_),
          routes(routes_){};

    // never null
    StopArea* start;
    // never null
    StopArea* end;

    std::vector<StopArea*> blockeds;

    // if start and end are both not blocked      : contains [start, blockeds, end]
    // if start is blocked and end is not blocked : contains [blockeds, end]
    // if start is not blocked and end is blocked : contains [start, blockeds]
    // if start and end are both blocked          : contains [blockeds]
    std::vector<StopArea*> impacted_stop_areas;

    // may be null if not line was given
    Line* line;

    // never empty
    std::vector<Route*> routes;

    bool is_blocked_start_point() const;
    bool is_start_stop(const std::string& uri) const;
    bool is_blocked_end_point() const;
    bool is_end_stop(const std::string& uri) const;

    bool impacts(const VehicleJourney* vj) const;

    template <class archive>
    void serialize(archive& ar, const unsigned int);
};

boost::optional<RailSection> try_make_rail_section(
    const navitia::type::PT_Data& pt_data,
    const std::string& start_uri,
    const std::vector<std::pair<std::string, uint32_t>>& blockeds_uri_order,
    const std::string& end_uri,
    const boost::optional<std::string>& line_uri,  // may be null
    const std::vector<std::string>& routes_uris    // may be empty
);

std::set<StopPoint*> get_stop_points_section(const RailSection& rs);

using PtObj = boost::variant<UnknownPtObj,
                             Network*,
                             StopArea*,
                             StopPoint*,
                             LineSection,
                             RailSection,
                             Line*,
                             Route*,
                             MetaVehicleJourney*>;

PtObj make_pt_obj(Type_e type, const std::string& uri, PT_Data& pt_data);

struct Message {
    std::string text;
    std::string channel_id;
    std::string channel_name;
    std::string channel_content_type;

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    std::set<ChannelType> channel_types;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct StopTimeUpdate {
    StopTime stop_time;
    std::string cause;  // TODO factorize this cause with a pool
    enum class Status : uint8_t {
        // Note: status are ordered, from least to most important
        UNCHANGED = 0,
        ADDED,
        ADDED_FOR_DETOUR,
        DELETED,
        DELETED_FOR_DETOUR,
        DELAYED
    };
    Status departure_status{Status::UNCHANGED};
    Status arrival_status{Status::UNCHANGED};
    StopTimeUpdate() = default;
    StopTimeUpdate(StopTime st, std::string c, Status dep_status, Status arr_status)
        : stop_time(std::move(st)), cause(std::move(c)), departure_status(dep_status), arrival_status(arr_status) {}

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

inline bool operator<(StopTimeUpdate::Status a, StopTimeUpdate::Status b) {
    using T = std::underlying_type<StopTimeUpdate::Status>::type;
    return static_cast<T>(a) < static_cast<T>(b);
}

namespace detail {
struct AuxInfoForMetaVJ {
    std::vector<StopTimeUpdate> stop_times;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
    const StopTimeUpdate& get_stop_time_update(const RankStopTime& order) const { return stop_times.at(order.val); }
};
}  // namespace detail

struct TimeSlot {
    uint32_t begin = 0;  ///< seconds since midnight
    uint32_t end = 0;    ///< seconds since midnight
    TimeSlot(uint32_t begin = 0, uint32_t end = 0) : begin{begin}, end{end} {}

    bool operator<(const TimeSlot& other) const;
    bool operator==(const TimeSlot& other) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct ApplicationPattern {
    navitia::type::WeekPattern week_pattern;
    boost::gregorian::date_period application_period{boost::gregorian::date(), boost::gregorian::date()};
    std::set<TimeSlot, std::less<TimeSlot>> time_slots;

    void add_time_slot(uint32_t begin, uint32_t end);
    bool operator<(const ApplicationPattern& other) const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct Impact {
    using SharedImpact = boost::shared_ptr<Impact>;
    std::string uri;
    std::string company_id;
    std::string physical_mode_id;
    std::string headsign;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    // the application period define when the impact happen
    // i.e. the canceled base schedule period for vj
    std::vector<boost::posix_time::time_period> application_periods;

    std::set<ApplicationPattern, std::less<ApplicationPattern>> application_patterns;

    boost::shared_ptr<Severity> severity;

    std::vector<Message> messages;

    detail::AuxInfoForMetaVJ aux_info;

    // link to the parent disruption
    // Note: it is a raw pointer because an Impact is owned by it's disruption
    //(even if the impact is stored as a share_ptr in the disruption to allow for weak_ptr towards it)
    Disruption* disruption;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    boost::iterator_range<std::vector<PtObj>::const_iterator> informed_entities() const {
        return boost::make_iterator_range(_informed_entities.begin(), _informed_entities.end());
    }
    boost::iterator_range<std::vector<PtObj>::iterator> mut_informed_entities() {
        return boost::make_iterator_range(_informed_entities.begin(), _informed_entities.end());
    }

    // add the ptobj to the informed entities and make all the needed backref
    // Note: it's a static method because we need the shared_ptr to the impact
    static void link_informed_entity(PtObj ptobj,
                                     SharedImpact& impact,
                                     const boost::gregorian::date_period&,
                                     type::RTLevel,
                                     type::PT_Data& pt_data);

    bool is_valid(const boost::posix_time::ptime& publication_date,
                  const boost::posix_time::time_period& active_period) const;
    bool is_relevant(const std::vector<const StopTime*>& stop_times) const;
    bool is_only_line_section() const;
    bool is_line_section_of(const Line&) const;
    bool is_only_rail_section() const;
    bool is_rail_section_of(const Line&) const;
    ActiveStatus get_active_status(const boost::posix_time::ptime& publication_date) const;
    Indexes get(Type_e target, PT_Data& pt_data) const;
    const type::ValidityPattern get_impact_vp(const boost::gregorian::date_period& production_date) const;

    bool operator<(const Impact& other);

private:
    std::vector<PtObj> _informed_entities;
};

struct Tag {
    std::string uri;
    std::string name;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

class DisruptionHolder;

struct Disruption {
    Disruption() = default;
    Disruption(const std::string u, RTLevel lvl) : uri(u), rt_level(lvl) {}
    Disruption& operator=(const Disruption&) = delete;
    Disruption(const Disruption&) = delete;

    std::string uri;
    // Provider of the disruption
    std::string contributor;
    // it's the title of the disruption as shown in the backoffice
    std::string reference;
    RTLevel rt_level = RTLevel::Adapted;

    // the publication period specify when an information can be displayed to
    // the customer, if a request is made before or after this period the
    // disruption must not be shown
    boost::posix_time::time_period publication_period{
        boost::posix_time::not_a_date_time,
        boost::posix_time::seconds(1)};  // no default constructor for time_period, we must provide a value

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    boost::shared_ptr<Cause> cause;

    // the place where the disruption happen, the impacts can be in anothers places
    std::vector<PtObj> localization;

    // additional informations on the disruption
    std::vector<boost::shared_ptr<Tag>> tags;

    // properties linked to the disruption
    std::set<Property> properties;

    std::string note;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

    void add_impact(const Impact::SharedImpact& impact, DisruptionHolder& holder);
    const std::vector<Impact::SharedImpact>& get_impacts() const { return impacts; }

    bool is_publishable(const boost::posix_time::ptime& current_time) const;

private:
    // Disruption have the ownership of the Impacts.  Impacts are
    // shared_ptr and not unique_ptr because there are weak_ptr
    // pointing to them in the impacted objects
    std::vector<Impact::SharedImpact> impacts;
};

class DisruptionHolder {
    std::map<std::string, std::unique_ptr<Disruption>> disruptions_by_uri;
    std::vector<boost::weak_ptr<Impact>> weak_impacts;

public:
    Disruption& make_disruption(const std::string& uri, type::RTLevel lvl);
    std::unique_ptr<Disruption> pop_disruption(const std::string& uri);
    const Disruption* get_disruption(const std::string& uri) const;
    size_t nb_disruptions() const { return disruptions_by_uri.size(); }
    void add_weak_impact(const boost::weak_ptr<Impact>&);
    void clean_weak_impacts();
    void forget_vj(const VehicleJourney*);
    const std::vector<boost::weak_ptr<Impact>>& get_weak_impacts() const { return weak_impacts; }
    boost::weak_ptr<Impact> get_weak_impact(size_t id) const { return weak_impacts[id]; }
    boost::shared_ptr<Impact> get_impact(size_t id) const { return weak_impacts[id].lock(); }
    // causes, severities and tags are a pool (weak_ptr because the owner ship
    // is in the linked disruption or impact)
    std::map<std::string, boost::weak_ptr<Cause>> causes;         // to be wrapped
    std::map<std::string, boost::weak_ptr<Severity>> severities;  // to be wrapped too
    std::map<std::string, boost::weak_ptr<Tag>> tags;             // to be wrapped too

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};

struct ImpactedVJ {
    std::string vj_uri;  // uri of the impacted vj
    ValidityPattern new_vp;
    std::set<RankStopTime> impacted_ranks;
    ImpactedVJ(std::string vj_uri, ValidityPattern vp, std::set<RankStopTime> r)
        : vj_uri(std::move(vj_uri)), new_vp(std::move(vp)), impacted_ranks(std::move(r)) {}
};
/*
 * return the list of vehicle journey that are impacted by the linesection
 */
std::vector<ImpactedVJ> get_impacted_vehicle_journeys(const LineSection&,
                                                      const Impact&,
                                                      const boost::gregorian::date_period&,
                                                      type::RTLevel);
/*
 * return the list of vehicle journey that are impacted by the railsection
 */
std::vector<ImpactedVJ> get_impacted_vehicle_journeys(const RailSection& rs,
                                                      const Impact& impact,
                                                      const boost::gregorian::date_period& production_period,
                                                      type::RTLevel rt_level);

std::pair<BlockedSAList, ConcatenateBlockedSASequence> create_blocked_sa_sequence(const RailSection& rs);
bool is_route_to_impact_content_sa_list(const BlockedSAList& blocked_sa_uri_sequence,
                                        const boost::container::flat_set<StopArea*>& stop_area_list);
bool blocked_sa_sequence_matching(const ConcatenateBlockedSASequence& concatenate_blocked_sa_uri_sequence_string,
                                  const navitia::type::VehicleJourney& vj,
                                  const std::set<RankStopTime>& st_rank_list);

}  // namespace disruption

}  // namespace type
}  // namespace navitia
