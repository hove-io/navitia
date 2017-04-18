#pragma once

#include "type/datetime.h"
#include "type/kirin.pb.h"
#include "type/response.pb.h"
#include "routing/raptor.h"
#include "kraken/realtime.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <set>
#include <vector>
#include <map>

/*
 * Utilities for tests
 */
namespace navitia { namespace test {

inline void handle_realtime_test(const std::string& id,
                                 const boost::posix_time::ptime& timestamp,
                                 const transit_realtime::TripUpdate& trip_update,
                                 const type::Data& data,
                                 std::unique_ptr<navitia::routing::RAPTOR>& raptor) {
    navitia::handle_realtime(id, timestamp, trip_update, data);
    data.dataRaptor->load(*data.pt_data);
    raptor = std::move(std::make_unique<navitia::routing::RAPTOR>(data));
}

inline uint64_t to_posix_timestamp(const std::string& str) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(str));
}

struct DelayedTimeStop {
    std::string _stop_name;
    int _arrival_time = 0;
    int _departure_time = 0;
    navitia::time_duration _arrival_delay = 0_s;
    navitia::time_duration _departure_delay = 0_s;
    std::string _msg = "birds on the tracks";
    bool _skipped = false;
    DelayedTimeStop(const std::string& n, int arrival_time, int departure_time):
        _stop_name(n), _arrival_time(arrival_time), _departure_time(departure_time) {}
    DelayedTimeStop(const std::string& n, int time):
        _stop_name(n), _arrival_time(time), _departure_time(time) {}

    DelayedTimeStop& arrival_delay(navitia::time_duration delay) { _arrival_delay = delay; return *this; }
    DelayedTimeStop& departure_delay(navitia::time_duration delay) { _departure_delay = delay; return *this; }
    DelayedTimeStop& delay(navitia::time_duration delay) { return arrival_delay(delay).departure_delay(delay); }
    DelayedTimeStop& skipped() { _skipped = true; return *this; }
};

inline transit_realtime::TripUpdate
make_delay_message(const std::string& vj_uri,
        const std::string& start_date,
        const std::vector<DelayedTimeStop>& delayed_time_stops) {
    transit_realtime::TripUpdate trip_update;
    auto trip = trip_update.mutable_trip();
    trip->set_trip_id(vj_uri);
    // start_date is used to disambiguate trips that are very late, cf:
    // https://github.com/CanalTP/chaos-proto/blob/master/gtfs-realtime.proto#L459
    trip->set_start_date(start_date);
    trip->set_schedule_relationship(transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED);
    auto st_update = trip_update.mutable_stop_time_update();

    for (const auto& delayed_st: delayed_time_stops) {
        auto stop_time = st_update->Add();
        auto arrival = stop_time->mutable_arrival();
        auto departure = stop_time->mutable_departure();
        stop_time->SetExtension(kirin::stoptime_message, delayed_st._msg);
        stop_time->set_stop_id(delayed_st._stop_name);
        arrival->set_time(delayed_st._arrival_time);
        arrival->set_delay(delayed_st._arrival_delay.total_seconds());
        departure->set_time(delayed_st._departure_time);
        departure->set_delay(delayed_st._departure_delay.total_seconds());
        if (delayed_st._skipped) {
            stop_time->set_schedule_relationship(
                        transit_realtime::TripUpdate_StopTimeUpdate_ScheduleRelationship_SKIPPED);
        }
    }

    return trip_update;
}

inline const pbnavitia::Impact*
get_impact(const std::string& uri, const pbnavitia::Response& resp) {
    for (const auto& impact: resp.impacts()) {
        if (impact.uri() == uri) { return &impact; }
    }
    return nullptr;
}

}}// namespace navitia::test

inline u_int32_t operator"" _t(const char* str, size_t s) {
    return boost::posix_time::duration_from_string(std::string(str, s)).total_seconds();
}

inline u_int64_t operator"" _pts(const char* str, size_t s) {
    return navitia::to_posix_timestamp(boost::posix_time::from_iso_string(std::string(str, s)));
}

namespace std {
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    auto it = s.cbegin(), end = s.cend();
    if (it != end) os << *it++;
    for (; it != end; ++it) os << ", " << *it;
    return os << "}";
}
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    auto it = v.cbegin(), end = v.cend();
    if (it != end) { os << *it++; }
    for (; it != end; ++it) { os << ", " << *it; }
    return os << "]";
}

template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p) {
    return os << "(" << p.first << ", " << p.second << ")";
}


template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
    os << "{";
    auto it = m.cbegin(), end = m.cend();
    if (it != end) { os << *it++; }
    for (; it != end; ++it) { os << ", " << *it; }
    return os << "}";
}
}// namespace std

// small helper especially usefull for ptref tests
// get list of pointers from indexes
template <typename T>
std::set<const T*> get_objects(const nt::Indexes& indexes, const nt::Data& data) {
    const auto& obj_col = data.get_data<T>();
    std::set<const T*> objs;
    for (const auto idx: indexes) {
        objs.insert(obj_col[idx]);
    }
    return objs;
}

// get uris from indexes
template <typename T>
std::set<std::string> get_uris(const nt::Indexes& indexes, const nt::Data& data) {
    const auto& objs = get_objects<T>(indexes, data);
    std::set<std::string> uris;
    for (const auto* obj: objs) {
        uris.insert(obj->uri);
    }
    return uris;
}


/*
 * BOOST_CHECK_EQUAL_COLLECTIONS does not work well with a temporary collection, thus this macro
 * (and it's easier to use)
 */
#define BOOST_CHECK_EQUAL_RANGE(range1, range2) \
    { \
        const auto& r1 = range1; \
        const auto& r2 = range2; \
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(r1), std::end(r1), std::begin(r2), std::end(r2)); \
    }

// maccro to test that a validity pattern ends with a sequence
#define BOOST_CHECK_END_VP(vp, sequence) \
    BOOST_CHECK_MESSAGE(boost::algorithm::ends_with(vp->days.to_string(), sequence), vp);
