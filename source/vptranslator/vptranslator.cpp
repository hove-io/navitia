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

#include "vptranslator/vptranslator.h"

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/for_each.hpp>

#include <unordered_map>

namespace navitia {
namespace vptranslator {

using navitia::type::ValidityPattern;
using Week = navitia::type::WeekPattern;
using boost::gregorian::date;
using boost::gregorian::date_duration;
using boost::gregorian::date_period;
using boost::gregorian::day_iterator;
using navitia::type::ExceptionDate;

template <typename T>
static std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    auto it = s.cbegin(), end = s.cend();
    if (it != end) {
        os << *it++;
    }
    for (; it != end; ++it) {
        os << ", " << *it;
    }
    return os << "}";
}

// a week index is 0 for Monday, 1 for Tuesday, ..., 6 for Sunday
static unsigned to_week_index(const date& d) {
    return navitia::get_weekday(d);
}
// returns the index after the last valid day, 0 if none
static unsigned end_active(const ValidityPattern& vp) {
    if (vp.days.size() == 0) {
        return 0;
    }
    unsigned i = vp.days.size() - 1;
    do {
        if (vp.check(i)) {
            return i + 1;
        }
    } while (i-- != 0);
    return 0;
}

void BlockPattern::canonize_validity_periods() {
    // As std::set is ordered and the periods in validity_periods must
    // not overlap, we can simply merge 2 consecutive period if the
    // end day of the first period is the same as the begin day of the
    // second period
    if (validity_periods.empty()) {
        return;
    }
    std::set<boost::gregorian::date_period> res;
    auto it = validity_periods.cbegin(), end = validity_periods.cend();
    auto cur_period = *it;
    for (++it; it != end; ++it) {
        if (cur_period.end() == it->begin()) {
            cur_period = date_period(cur_period.begin(), it->end());
        } else {
            res.insert(cur_period);
            cur_period = *it;
        }
    }
    res.insert(cur_period);
    validity_periods = res;
}

unsigned BlockPattern::nb_weeks() const {
    unsigned res = 0;
    for (const auto& p : validity_periods) {
        res += p.length().days() / 7;
        const auto idx_begin = to_week_index(p.begin());
        const auto idx_end = to_week_index(p.end());
        if (idx_begin == 0) {
            // adding the partial week at the end
            if (idx_end != 0) {
                res += 1;
            }
        } else if (idx_end == 0) {
            // adding the partial week at the begining
            if (idx_begin != 0) {
                res += 1;
            }
        } else if (idx_begin > idx_end) {
            // we have 2 partial weeks (at the begining and at the
            // end), and the number of days in the two partial weeks
            // are less than a week, thus, we should add 2
            res += 2;
        } else {
            // we have 2 partial weeks and the number of days in the
            // two partial weeks are greater or equal to one week,
            // thus, one week is already added by the division, we
            // just have to add one week.
            res += 1;
        }
    }
    return res;
}

void BlockPattern::add_from(const BlockPattern& other) {
    if (!other.exceptions.empty()) {
        throw std::invalid_argument("add_from takes only no exception block patterns");
    }
    const Week diff = week ^ other.week;
    const Week to_exclude = ~other.week & diff;
    const Week to_include = other.week & diff;
    for (const auto& period : other.validity_periods) {
        validity_periods.insert(period);
        for (day_iterator day_it = period.begin(); day_it != period.end(); ++day_it) {
            const auto week_idx = to_week_index(*day_it);
            if (to_exclude[week_idx]) {
                exceptions.insert({ExceptionDate::ExceptionType::sub, *day_it});
            }
            if (to_include[week_idx]) {
                exceptions.insert({ExceptionDate::ExceptionType::add, *day_it});
            }
        }
    }
}

std::ostream& operator<<(std::ostream& os, const BlockPattern& bp) {
    if (bp.week[0]) {
        os << "Mo";
    }
    if (bp.week[1]) {
        os << "Tu";
    }
    if (bp.week[2]) {
        os << "We";
    }
    if (bp.week[3]) {
        os << "Th";
    }
    if (bp.week[4]) {
        os << "Fr";
    }
    if (bp.week[5]) {
        os << "Sa";
    }
    if (bp.week[6]) {
        os << "Su";
    }
    if (bp.week.none()) {
        os << "none";
    }
    os << " for " << bp.validity_periods;
    if (!bp.exceptions.empty()) {
        os << " except " << bp.exceptions;
    }
    return os;
}

namespace {
struct DistanceMatrix : boost::noncopyable {
    // we have 2^7 days in a week (bit shift as we need a constexpr)
    static const unsigned size = 1u << 7u;
    using Line = std::array<unsigned, size>;
    using Matrix = std::array<Line, size>;
    Matrix array{};
    friend const DistanceMatrix& distance_matrix();
    unsigned score(unsigned i, const Line& l) const {
        unsigned res = 0;
        boost::for_each(array[i], l, [&](unsigned i, unsigned j) { res += i * j; });
        return res;
    }

private:
    DistanceMatrix() {
        for (unsigned i = 0; i < size; ++i) {
            Week w1(i);
            for (unsigned j = 0; j < size; ++j) {
                Week w2(j);
                array[i][j] = (w1 ^ w2).count();
            }
        }
    }
};
const DistanceMatrix& distance_matrix() {
    // With private constructor, noncopyable and this friend function,
    // we are sure to only compute the DistanceMatrix once.
    static const DistanceMatrix d;
    return d;
}
DistanceMatrix::Line make_line(const std::vector<BlockPattern>& bps) {
    DistanceMatrix::Line line;
    line.fill(0);
    for (const auto& bp : bps) {
        line[bp.week.to_ulong()] += bp.nb_weeks();
    }
    return line;
}
Week get_min_week_pattern(const std::vector<BlockPattern>& bps) {
    const auto line = make_line(bps);
    const auto& matrix = distance_matrix();
    unsigned best = 0;
    unsigned best_score = matrix.score(0, line);
    for (unsigned i = 1; i < DistanceMatrix::size; ++i) {
        const unsigned cur_score = matrix.score(i, line);
        if (cur_score < best_score ||
            // in case of equality, we prefere more setted bits as it
            // may be better for partial week bounds
            (cur_score == best_score && Week(i).count() > Week(best).count())) {
            best = i;
            best_score = cur_score;
        }
    }
    return {best};
}
}  // namespace

BlockPattern translate_one_block(const navitia::type::ValidityPattern& vp) {
    const std::vector<BlockPattern> no_except = translate_no_exception(vp);
    switch (no_except.size()) {
        case 0:
            return BlockPattern();
        case 1:
            return no_except.front();
        default:
            break;  // done after
    }
    BlockPattern res;
    res.week = get_min_week_pattern(no_except);
    for (const auto& bp : no_except) {
        res.add_from(bp);
    }
    res.canonize_validity_periods();
    return res;
}

std::vector<BlockPattern> translate_no_exception(const ValidityPattern& vp) {
    unsigned offset = 0, end = end_active(vp);
    std::unordered_map<Week, BlockPattern> patterns;

    // skipping zeros at the beginning
    for (offset = 0; offset < end && !vp.check(offset); ++offset) {
    }

    date begin_period = vp.beginning_date + date_duration(offset);
    Week cur_week_pattern;
    for (; offset < end; ++offset) {
        const auto cur_day = vp.beginning_date + date_duration(offset);
        if (vp.check(offset)) {
            cur_week_pattern.set(to_week_index(cur_day));
        }
        if (to_week_index(cur_day) == 6) {
            if (cur_week_pattern.any()) {
                auto& pat = patterns[cur_week_pattern];
                pat.week = cur_week_pattern;
                pat.validity_periods.insert(date_period(begin_period, cur_day + date_duration(1)));
            }
            begin_period = cur_day + date_duration(1);
            cur_week_pattern.reset();
        }
    }

    // last incomplete period
    if (cur_week_pattern.any()) {
        const auto end_day = vp.beginning_date + date_duration(offset);
        auto& pat = patterns[cur_week_pattern];
        pat.week = cur_week_pattern;
        pat.validity_periods.insert(date_period(begin_period, end_day));
    }

    std::vector<BlockPattern> res;
    for (const auto& p : patterns) {
        res.emplace_back(p.second);
        res.back().canonize_validity_periods();
    }
    using BP = BlockPattern;
    boost::sort(res, [](const BP& a, const BP& b) {
        assert(!a.validity_periods.empty());
        assert(!b.validity_periods.empty());
        return a.validity_periods.begin()->begin() < b.validity_periods.begin()->begin();
    });
    return res;
}

std::vector<BlockPattern> translate(const navitia::type::ValidityPattern& vp) {
    auto res = translate_one_block(vp);
    if (res.validity_periods.empty()) {
        return {};
    }
    return {res};
}

}  // namespace vptranslator
}  // namespace navitia
