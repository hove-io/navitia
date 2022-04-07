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

#include <boost/date_time/time_duration.hpp>
#include <boost/date_time/time_resolution_traits.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/parsers.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/serialization/split_free.hpp>

#include <iomanip>

// adapted from boost (I don't understand why it is not like that in boost
namespace boost {
namespace date_time {

template <class T, typename rep_type>
inline std::string to_simple_string(time_duration<T, rep_type> td) {
    std::ostringstream ss;
    if (td.is_special()) {
        switch (td.get_rep().as_special()) {
            case not_a_date_time:
                ss << "not-a-date-time";
                break;
            case pos_infin:
                ss << "+infinity";
                break;
            case neg_infin:
                ss << "-infinity";
                break;
            default:
                ss << "";
        }
    } else {
        if (td.is_negative()) {
            ss << '-';
        }
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.hours()) << ":";
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.minutes()) << ":";
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.seconds());
        // TODO the following is totally non-generic, yelling FIXME
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
        boost::int64_t frac_sec = date_time::absolute_value(td.fractional_seconds());
        // JDG [7/6/02 VC++ compatibility]
        char buff[32];
        _i64toa(frac_sec, buff, 10);
#else
        typename time_duration<T, rep_type>::fractional_seconds_type frac_sec = absolute_value(td.fractional_seconds());
#endif
        if (frac_sec != 0) {
            ss << "." << std::setw(time_duration<T, rep_type>::num_fractional_digits()) << std::setfill('0')

            // JDG [7/6/02 VC++ compatibility]
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
               << buff;
#else
               << frac_sec;
#endif
        }
    }  // else
    return ss.str();
}

//! Time duration in iso format -hhmmss,fffffff Example: 10:09:03,0123456
/*!\ingroup time_format
 */
template <class T, typename rep_type>
inline std::string to_iso_string(time_duration<T, rep_type> td) {
    std::ostringstream ss;
    if (td.is_special()) {
        /* simply using 'ss << td.get_rep()' won't work on compilers
         * that don't support locales. This way does. */
        // switch copied from date_names_put.hpp
        switch (td.get_rep().as_special()) {
            case not_a_date_time:
                // ss << "not-a-number";
                ss << "not-a-date-time";
                break;
            case pos_infin:
                ss << "+infinity";
                break;
            case neg_infin:
                ss << "-infinity";
                break;
            default:
                ss << "";
        }
    } else {
        if (td.is_negative()) {
            ss << '-';
        }
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.hours());
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.minutes());
        ss << std::setw(2) << std::setfill('0') << absolute_value(td.seconds());
        // TODO the following is totally non-generic, yelling FIXME
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
        boost::int64_t frac_sec = date_time::absolute_value(td.fractional_seconds());
        // JDG [7/6/02 VC++ compatibility]
        char buff[32];
        _i64toa(frac_sec, buff, 10);
#else
        typename time_duration<T, rep_type>::fractional_seconds_type frac_sec = absolute_value(td.fractional_seconds());
#endif
        if (frac_sec != 0) {
            ss << "." << std::setw(time_duration<T, rep_type>::num_fractional_digits()) << std::setfill('0')

            // JDG [7/6/02 VC++ compatibility]
#if (defined(BOOST_MSVC) && (_MSC_VER < 1300))
               << buff;
#else
               << frac_sec;
#endif
        }
    }  // else
    return ss.str();
}
}  // namespace date_time
}  // namespace boost

namespace navitia {

// traits struct for time_resolution_traits using unsigned int32
struct time_resolution_traits_adapted_int32_impl {
    using int_type = int32_t;
    using impl_type = boost::date_time::int_adapter<int_type>;
    static int_type as_number(impl_type i) { return i.as_number(); }
    static bool is_adapted() { return true; }
};

using time_res_traits =
    boost::date_time::time_resolution_traits<time_resolution_traits_adapted_int32_impl, boost::date_time::tenth, 10, 1>;

/**
 * For georef we only need tenth fo seconds precision, milli, micro or nano second are not needed.
 *
 * We thus can store the duration in an int32 instead of a int64.
 *
 * We also add float division (we need it for speed modifier computation)
 *
 * TODO: we could also add time_duration on int16 for the street network graph
 * (with a second precision, it would handle ~9h)
 */
class time_duration : public boost::date_time::time_duration<time_duration, time_res_traits> {
public:
    using rep_type = time_res_traits;
    using day_type = time_res_traits::day_type;
    using hour_type = time_res_traits::hour_type;
    using min_type = time_res_traits::min_type;
    using sec_type = time_res_traits::sec_type;
    using fractional_seconds_type = time_res_traits::fractional_seconds_type;
    using tick_type = time_res_traits::tick_type;
    using impl_type = time_res_traits::impl_type;

    time_duration(hour_type hour, min_type min, sec_type sec, fractional_seconds_type fs = 0)
        : boost::date_time::time_duration<time_duration, time_res_traits>(hour, min, sec, fs) {}
    time_duration() : boost::date_time::time_duration<time_duration, time_res_traits>(0, 0, 0) {}
    //! Construct from special_values
    time_duration(boost::date_time::special_values sv)
        : boost::date_time::time_duration<time_duration, time_res_traits>(sv) {}

    // constructor for a 'classic' posix time_duration
    static time_duration from_boost_duration(boost::posix_time::time_duration d) {
        if (d.is_special()) {
            if (d.is_pos_infinity()) {
                return {boost::date_time::pos_infin};
            }
            if (d.is_neg_infinity()) {
                return {boost::date_time::neg_infin};
            }
            if (d.is_not_a_date_time()) {
                return {boost::date_time::not_a_date_time};
            }
            throw navitia::exception("unhandled case for duration construction");
        }
        auto dur = time_duration(d.hours(), d.minutes(), d.seconds(), fractional_seconds_type(d.fractional_seconds()));

        // check for overflow
        if (dur.total_seconds() != d.total_seconds()) {
            throw navitia::exception("duration overflow while converting from boost::time_duration: "
                                     + boost::date_time::to_simple_string(d));
        }
        return dur;
    }
    boost::posix_time::time_duration to_posix() const { return {hours(), minutes(), seconds(), fractional_seconds()}; }

    // TODO
    // benchmark if a build-in constructor with bt::pos_infin improve perf

    friend class boost::date_time::time_duration<time_duration, time_res_traits>;

    // float division (rounding is done on the ticks)
    time_duration operator/(float divisor) const {
        if (divisor == 0.0f) {
            throw navitia::exception("cannot divide duration by 0");
        }
        return time_duration(ticks_.as_number() / divisor);
    }

    time_duration operator*(float multiplicator) const { return time_duration(ticks_.as_number() * multiplicator); }

    float total_fractional_seconds() const { return float(ticks()) / ticks_per_second(); }

private:
    explicit time_duration(impl_type tick_count)
        : boost::date_time::time_duration<time_duration, time_res_traits>(tick_count) {}
};

// mirror boost helpers
class hours : public time_duration {
public:
    explicit hours(time_res_traits::hour_type h) : time_duration(h, 0, 0) {}
};

class minutes : public time_duration {
public:
    explicit minutes(time_res_traits::min_type m) : time_duration(0, m, 0) {}
};

class seconds : public time_duration {
public:
    explicit seconds(time_res_traits::sec_type s) : time_duration(0, 0, s) {}
};

class milliseconds : public time_duration {
public:
    explicit milliseconds(time_res_traits::fractional_seconds_type ms)
        : time_duration(0, 0, 0, ms * time_res_traits::res_adjust() / 1000) {}
};

// serialization functions (copied from boost)
template <class Archive>
void save(Archive& ar, const time_duration& td, unsigned int /*version*/) {
    // serialize a bool so we know how to read this back in later
    bool is_special = td.is_special();
    ar& boost::serialization::make_nvp("is_special", is_special);
    if (is_special) {
        std::string s = boost::date_time::to_simple_string(td);
        ar& boost::serialization::make_nvp("sv_time_duration", s);
    } else {
        typename time_duration::hour_type h = td.hours();
        typename time_duration::min_type m = td.minutes();
        typename time_duration::sec_type s = td.seconds();
        typename time_duration::fractional_seconds_type fs = td.fractional_seconds();
        ar& boost::serialization::make_nvp("time_duration_hours", h);
        ar& boost::serialization::make_nvp("time_duration_minutes", m);
        ar& boost::serialization::make_nvp("time_duration_seconds", s);
        ar& boost::serialization::make_nvp("time_duration_fractional_seconds", fs);
    }
}

template <class Archive>
void load(Archive& ar, time_duration& td, unsigned int /*version*/) {
    bool is_special = false;
    ar& boost::serialization::make_nvp("is_special", is_special);
    if (is_special) {
        std::string s;
        ar& boost::serialization::make_nvp("sv_time_duration", s);
        auto sv = boost::gregorian::special_value_from_string(s);
        td = time_duration(sv);
    } else {
        typename time_duration::hour_type h(0);
        typename time_duration::min_type m(0);
        typename time_duration::sec_type s(0);
        typename time_duration::fractional_seconds_type fs(0);
        ar& boost::serialization::make_nvp("time_duration_hours", h);
        ar& boost::serialization::make_nvp("time_duration_minutes", m);
        ar& boost::serialization::make_nvp("time_duration_seconds", s);
        ar& boost::serialization::make_nvp("time_duration_fractional_seconds", fs);
        td = time_duration(h, m, s, fs);
    }
}

inline std::ostream& operator<<(std::ostream& os, time_duration t) {
    os << boost::date_time::to_simple_string(t);
    return os;
}
}  // namespace navitia

BOOST_SERIALIZATION_SPLIT_FREE(navitia::time_duration)
