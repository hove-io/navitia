#pragma once

#include "type/datetime.h"
#include "type/kirin.pb.h"
#include "type/response.pb.h"
#include "routing/raptor.h"
#include "kraken/realtime.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <set>
#include <utility>

#include <vector>
#include <map>
#include <boost/range/algorithm/transform.hpp>

/*
 * Utilities for tests
 */
namespace navitia {
namespace test {

inline void handle_realtime_test(const std::string& id,
                                 const boost::posix_time::ptime& timestamp,
                                 const transit_realtime::TripUpdate& trip_update,
                                 const type::Data& data,
                                 std::unique_ptr<navitia::routing::RAPTOR>& raptor) {
    navitia::handle_realtime(id, timestamp, trip_update, data, true, true);
    data.dataRaptor->load(*data.pt_data);
    raptor = std::make_unique<navitia::routing::RAPTOR>(data);
}

inline std::set<std::string> get_impacts_uris(
    const std::set<navitia::type::disruption::Impact::SharedImpact, Less>& impacts) {
    std::set<std::string> uris;
    boost::range::transform(
        impacts, std::inserter(uris, uris.begin()),
        [](const navitia::type::disruption::Impact::SharedImpact& i) { return i->disruption->uri; });

    return uris;
}

inline uint64_t to_posix_timestamp(const std::string& str) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(str));
}

inline int32_t to_int_date(const std::string& str) {
    return navitia::to_int_date(boost::posix_time::from_iso_string(str));
}

struct RTStopTime {
    std::string _stop_name;
    int _arrival_time = 0;
    int _departure_time = 0;
    navitia::time_duration _arrival_delay = 0_s;
    navitia::time_duration _departure_delay = 0_s;
    std::string _msg = "birds on the tracks";
    bool _departure_skipped = false;
    bool _arrival_skipped = false;
    bool _is_added = false;
    bool _deleted_for_detour = false;
    bool _added_for_detour = false;
    RTStopTime(std::string n, int arrival_time, int departure_time)
        : _stop_name(std::move(n)), _arrival_time(arrival_time), _departure_time(departure_time) {}
    RTStopTime(std::string n, int time) : _stop_name(std::move(n)), _arrival_time(time), _departure_time(time) {}

    RTStopTime& arrival_delay(navitia::time_duration delay) {
        _arrival_delay = delay;
        return *this;
    }
    RTStopTime& departure_delay(navitia::time_duration delay) {
        _departure_delay = delay;
        return *this;
    }
    RTStopTime& delay(navitia::time_duration delay) { return arrival_delay(delay).departure_delay(delay); }
    RTStopTime& arrival_skipped() {
        _arrival_skipped = true;
        return *this;
    }
    RTStopTime& departure_skipped() {
        _departure_skipped = true;
        return *this;
    }
    RTStopTime& skipped() { return arrival_skipped().departure_skipped(); }
    RTStopTime& added() {
        _is_added = true;
        return *this;
    }
    RTStopTime& deleted_for_detour() {
        _deleted_for_detour = true;
        return *this;
    }
    RTStopTime& added_for_detour() {
        _added_for_detour = true;
        return *this;
    }
};

inline transit_realtime::TripUpdate make_trip_update_message(
    const std::string& mvj_uri,
    const std::string& start_date,
    const std::vector<RTStopTime>& delayed_time_stops,
    const transit_realtime::Alert_Effect effect = transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS,
    const std::string& company_id = "",
    const std::string& physical_mode_id = "",
    const std::string& contributor = "",
    const std::string& trip_message = "",
    const std::string& headsign = "") {
    transit_realtime::TripUpdate trip_update;
    trip_update.SetExtension(kirin::effect, effect);
    auto trip = trip_update.mutable_trip();
    trip->set_trip_id(mvj_uri);
    if (company_id != "") {
        trip->SetExtension(kirin::company_id, company_id);
    }
    auto vehicle_descriptor = trip_update.mutable_vehicle();
    if (physical_mode_id != "") {
        vehicle_descriptor->SetExtension(kirin::physical_mode_id, physical_mode_id);
    }
    if (contributor != "") {
        trip->SetExtension(kirin::contributor, contributor);
    }
    if (trip_message != "") {
        trip_update.SetExtension(kirin::trip_message, trip_message);
    }
    if (headsign != "") {
        trip_update.SetExtension(kirin::headsign, headsign);
    }
    // start_date is used to disambiguate trips that are very late, cf:
    // https://github.com/hove-io/chaos-proto/blob/master/gtfs-realtime.proto#L459
    trip->set_start_date(start_date);
    auto st_update = trip_update.mutable_stop_time_update();

    for (const auto& delayed_st : delayed_time_stops) {
        auto stop_time = st_update->Add();
        auto arrival = stop_time->mutable_arrival();
        auto departure = stop_time->mutable_departure();
        stop_time->SetExtension(kirin::stoptime_message, delayed_st._msg);
        stop_time->set_stop_id(delayed_st._stop_name);
        arrival->set_time(delayed_st._arrival_time);
        arrival->set_delay(delayed_st._arrival_delay.total_seconds());
        departure->set_time(delayed_st._departure_time);
        departure->set_delay(delayed_st._departure_delay.total_seconds());
        if (delayed_st._departure_skipped) {
            departure->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::DELETED);
        }
        if (delayed_st._arrival_skipped) {
            arrival->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::DELETED);
        }
        if (delayed_st._is_added) {
            departure->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::ADDED);
            arrival->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::ADDED);
        }
        if (delayed_st._deleted_for_detour) {
            departure->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::DELETED_FOR_DETOUR);
            arrival->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::DELETED_FOR_DETOUR);
        }
        if (delayed_st._added_for_detour) {
            departure->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::ADDED_FOR_DETOUR);
            arrival->SetExtension(kirin::stop_time_event_status, kirin::StopTimeEventStatus::ADDED_FOR_DETOUR);
        }
    }

    return trip_update;
}

inline const pbnavitia::Impact* get_impact(const std::string& uri, const pbnavitia::Response& resp) {
    for (const auto& impact : resp.impacts()) {
        if (impact.uri() == uri) {
            return &impact;
        }
    }
    return nullptr;
}

}  // namespace test
}  // namespace navitia

inline u_int32_t operator"" _t(const char* str, size_t s) {
    return boost::posix_time::duration_from_string(std::string(str, s)).total_seconds();
}

inline u_int64_t operator"" _pts(const char* str, size_t s) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(std::string(str, s)));
}

namespace std {
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    auto it = s.cbegin(), end = s.cend();
    if (it != end)
        os << *it++;
    for (; it != end; ++it)
        os << ", " << *it;
    return os << "}";
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    auto it = v.cbegin(), end = v.cend();
    if (it != end) {
        os << *it++;
    }
    for (; it != end; ++it) {
        os << ", " << *it;
    }
    return os << "]";
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p) {
    return os << "(" << p.first << ", " << p.second << ")";
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
    os << "{";
    auto it = m.cbegin(), end = m.cend();
    if (it != end) {
        os << *it++;
    }
    for (; it != end; ++it) {
        os << ", " << *it;
    }
    return os << "}";
}
}  // namespace std

// small helper especially usefull for ptref tests
// get list of pointers from indexes
template <typename T>
std::set<const T*> get_objects(const nt::Indexes& indexes, const nt::Data& data) {
    const auto& obj_col = data.get_data<T>();
    std::set<const T*> objs;
    for (const auto idx : indexes) {
        objs.insert(obj_col[idx]);
    }
    return objs;
}

// get uris from indexes
template <typename T>
std::set<std::string> get_uris(const nt::Indexes& indexes, const nt::Data& data) {
    const auto& objs = get_objects<T>(indexes, data);
    std::set<std::string> uris;
    for (const auto* obj : objs) {
        uris.insert(obj->uri);
    }
    return uris;
}

/*
 * BOOST_CHECK_EQUAL_COLLECTIONS does not work well with a temporary collection, thus this macro
 * (and it's easier to use)
 */
#define BOOST_CHECK_EQUAL_RANGE(range1, range2)                                                    \
    {                                                                                              \
        const auto& r1 = range1;                                                                   \
        const auto& r2 = range2;                                                                   \
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(r1), std::end(r1), std::begin(r2), std::end(r2)); \
    }

// macro to test that a validity pattern ends with a sequence
// Note: it has been done as a macro and not a function to have a better error message
#define BOOST_CHECK_END_VP(vp, sequence) \
    BOOST_CHECK_MESSAGE(boost::algorithm::ends_with(vp->days.to_string(), sequence), vp);
