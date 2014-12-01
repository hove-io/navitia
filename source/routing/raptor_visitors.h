#pragma once

namespace navitia { namespace routing {
struct raptor_visitor {
    inline bool better_or_equal(const DateTime &a, const DateTime &current_dt, const type::StopTime& st) const {
        return a <= st.section_end_date(DateTimeUtils::date(current_dt), clockwise());
    }

    inline
    std::pair<std::vector<type::JourneyPatternPoint*>::const_iterator, std::vector<type::JourneyPatternPoint*>::const_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint*> &, const type::JourneyPattern* journey_pattern, size_t order) const {
        return std::make_pair(journey_pattern->journey_pattern_point_list.begin() + order,
                              journey_pattern->journey_pattern_point_list.end());
    }

    typedef std::vector<type::StopTime>::const_iterator stop_time_iterator;
    inline stop_time_iterator first_stoptime(const type::StopTime& st) const {
        const type::JourneyPatternPoint* jpp = st.journey_pattern_point;
        const type::VehicleJourney* vj = st.vehicle_journey;
        return vj->stop_time_list.begin() + jpp->order;
    }

    template<typename T1, typename T2> inline bool comp(const T1& a, const T2& b) const {
        return a < b;
    }

    template<typename T1, typename T2> inline auto combine(const T1& a, const T2& b) const -> decltype(a+b) {
        return a + b;
    }

    inline
    const type::VehicleJourney* get_extension_vj(const type::VehicleJourney* vj) const {
       return vj->next_vj;
    }

    inline
    const type::JourneyPatternPoint* get_last_jpp(const type::VehicleJourney* vj) const {
       if(vj->prev_vj) {
            return vj->prev_vj->journey_pattern->journey_pattern_point_list.back();
       } else {
           return nullptr;
       }
    }

    inline
    std::pair<std::vector<type::StopTime>::const_iterator, std::vector<type::StopTime>::const_iterator>
    stop_time_list(const type::VehicleJourney* vj) const {
        return std::make_pair(vj->stop_time_list.begin(), vj->stop_time_list.end());
    }

    constexpr bool clockwise() const{return true;}
    constexpr int init_queue_item() const{return std::numeric_limits<int>::max();}
    constexpr DateTime worst_datetime() const{return DateTimeUtils::inf;}
};


struct raptor_reverse_visitor {
    inline bool better_or_equal(const DateTime &a, const DateTime &current_dt, const type::StopTime& st) const {
        return a >= st.section_end_date(DateTimeUtils::date(current_dt), clockwise());
    }

    inline
    std::pair<std::vector<type::JourneyPatternPoint*>::const_reverse_iterator, std::vector<type::JourneyPatternPoint*>::const_reverse_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint*> &/*journey_pattern_points*/, const type::JourneyPattern* journey_pattern, size_t order) const {
        size_t offset = journey_pattern->journey_pattern_point_list.size() - order - 1;
        const auto begin = journey_pattern->journey_pattern_point_list.rbegin() + offset;
        const auto end = journey_pattern->journey_pattern_point_list.rend();
        return std::make_pair(begin, end);
    }

    typedef std::vector<type::StopTime>::const_reverse_iterator stop_time_iterator;
    inline stop_time_iterator first_stoptime(const type::StopTime& st) const {
        const type::JourneyPatternPoint* jpp = st.journey_pattern_point;
        const type::VehicleJourney* vj = st.vehicle_journey;
        return vj->stop_time_list.rbegin() + vj->stop_time_list.size() - jpp->order - 1;
    }

    template<typename T1, typename T2> inline bool comp(const T1& a, const T2& b) const {
        return a > b;
    }

    template<typename T1, typename T2> inline auto combine(const T1& a, const T2& b) const -> decltype(a-b) {
        return a - b;
    }

    inline
    const type::VehicleJourney* get_extension_vj(const type::VehicleJourney* vj) const {
       return vj->prev_vj;
    }

    inline
    const type::JourneyPatternPoint* get_last_jpp(const type::VehicleJourney* vj) const {
        if(vj->next_vj != nullptr) {
            return vj->next_vj->journey_pattern->journey_pattern_point_list.front();
        } else {
            return nullptr;
        }
    }

    inline
    std::pair<std::vector<type::StopTime>::const_reverse_iterator, std::vector<type::StopTime>::const_reverse_iterator>
    stop_time_list(const type::VehicleJourney* vj) const {
        return std::make_pair(vj->stop_time_list.rbegin(), vj->stop_time_list.rend());
    }

    constexpr bool clockwise() const{return false;}
    constexpr int init_queue_item() const{return -1;}
    constexpr DateTime worst_datetime() const{return DateTimeUtils::min;}
};

}}
