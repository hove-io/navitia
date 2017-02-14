#pragma once

#include <boost/range/iterator_range_core.hpp>

namespace navitia { namespace routing {
struct raptor_visitor {
    raptor_visitor() {}

    typedef std::vector<type::StopTime>::const_iterator stop_time_iterator;
    typedef boost::iterator_range<stop_time_iterator> stop_time_range;

    inline bool better_or_equal(const DateTime& a, const DateTime& current_dt, const type::StopTime& st, const DateTime& shift) const {
        return a <= (st.section_end(current_dt - shift, clockwise()) + shift);
    }

    inline boost::iterator_range<std::vector<dataRAPTOR::JppsFromJp::Jpp>::const_iterator>
    jpps_from_order(const dataRAPTOR::JppsFromJp& jpps_from_jp, JpIdx jp_idx, uint16_t jpp_order) const {
        const auto& jpps = jpps_from_jp[jp_idx];
        return boost::make_iterator_range(jpps.begin() + jpp_order, jpps.end());
    }

    inline stop_time_range st_range(const type::StopTime& st) const {
        const type::VehicleJourney* vj = st.vehicle_journey;
        return boost::make_iterator_range(vj->stop_time_list.begin() + st.order(),
                                          vj->stop_time_list.end());
    }

    template<typename T1, typename T2> inline bool comp(const T1& a, const T2& b) const {
        return a < b;
    }
    // better or equal
    template<typename T1, typename T2> inline bool be(const T1& a, const T2& b) const {
        return ! comp(b, a);
    }

    template<typename T1, typename T2> inline auto combine(const T1& a, const T2& b) const -> decltype(a+b) {
        return a + b;
    }

    inline
    const type::VehicleJourney* get_extension_vj(const type::VehicleJourney* vj) const {
       return vj->next_vj;
    }

    inline stop_time_range stop_time_list(const type::VehicleJourney* vj) const {
        return boost::make_iterator_range(vj->stop_time_list.begin(), vj->stop_time_list.end());
    }

    bool clockwise() const{return true;}
    StopEvent stop_event() const{return StopEvent::pick_up;}
    int init_queue_item() const{return std::numeric_limits<int>::max();}
    DateTime worst_datetime() const{return DateTimeUtils::inf;}
};


struct raptor_reverse_visitor {

    raptor_reverse_visitor() {}

    typedef std::vector<type::StopTime>::const_reverse_iterator stop_time_iterator;
    typedef boost::iterator_range<stop_time_iterator> stop_time_range;

    inline bool better_or_equal(const DateTime &a, const DateTime &current_dt, const type::StopTime& st, const DateTime& shift) const {
        return a >= (st.section_end(current_dt - shift, clockwise()) + shift);
    }

    inline boost::iterator_range<std::vector<dataRAPTOR::JppsFromJp::Jpp>::const_reverse_iterator>
    jpps_from_order(const dataRAPTOR::JppsFromJp& jpps_from_jp, JpIdx jp_idx, uint16_t jpp_order) const {
        const auto& jpps = jpps_from_jp[jp_idx];
        return boost::make_iterator_range(jpps.rbegin() + jpps.size() - jpp_order - 1, jpps.rend());
    }

    inline stop_time_range st_range(const type::StopTime& st) const {
        const type::VehicleJourney* vj = st.vehicle_journey;
        return boost::make_iterator_range(
            vj->stop_time_list.rbegin() + vj->stop_time_list.size() - st.order() - 1,
            vj->stop_time_list.rend());
    }

    template<typename T1, typename T2> inline bool comp(const T1& a, const T2& b) const {
        return a > b;
    }
    // better or equal
    template<typename T1, typename T2> inline bool be(const T1& a, const T2& b) const {
        return ! comp(b, a);
    }

    template<typename T1, typename T2> inline auto combine(const T1& a, const T2& b) const -> decltype(a-b) {
        return a - b;
    }

    inline
    const type::VehicleJourney* get_extension_vj(const type::VehicleJourney* vj) const {
       return vj->prev_vj;
    }

    inline stop_time_range stop_time_list(const type::VehicleJourney* vj) const {
        return boost::make_iterator_range(vj->stop_time_list.rbegin(), vj->stop_time_list.rend());
    }

    bool clockwise() const{return false;}
    StopEvent stop_event() const{return StopEvent::drop_off;}
    int init_queue_item() const{return -1;}
    DateTime worst_datetime() const{return DateTimeUtils::min;}
};

}}
