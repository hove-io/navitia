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

#include "utils/serialization_fusion_map.h"
#include "type/fwd_type.h"
#include "type/comment.h"

#include <boost/fusion/include/at_key.hpp>

#include <vector>
#include <map>
#include <type_traits>

namespace navitia {
namespace type {

/**
 * Comment container
 *
 * can associate a list of comment to various pt objects
 */
struct Comments {
    template <typename T>
    const std::vector<Comment> get(const T& obj) const {
        const auto& c =
            boost::fusion::at_key<typename std::remove_const<typename std::remove_pointer<T>::type>::type>(map);

        return find_or_default(get_as_key(obj), c);
    }

    template <typename T>
    void add(const T& obj, const Comment& comment) {
        auto& c = boost::fusion::at_key<typename std::remove_const<typename std::remove_pointer<T>::type>::type>(map);
        c[get_as_key(obj)].push_back(comment);
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& map;
    }

private:
    using comment_list = std::vector<Comment>;

    template <typename T>
    using pt_object_comment_map = std::map<const T*, comment_list>;

    template <typename T>
    using fusion_pair_comment_map = boost::fusion::pair<T, pt_object_comment_map<T>>;

    using stop_time_key = std::pair<const navitia::type::VehicleJourney*, RankStopTime>;

    template <typename T>
    const T* get_as_key(const T& obj) const {
        return &obj;
    }
    template <typename T>
    const T* get_as_key(T* const& obj) const {
        return obj;
    }
    template <typename T>
    const T* get_as_key(T const* const& obj) const {
        return obj;
    }
    const stop_time_key get_as_key(const navitia::type::StopTime& st) const;

    using comment_map_type =
        boost::fusion::map<fusion_pair_comment_map<navitia::type::StopArea>,
                           fusion_pair_comment_map<navitia::type::StopPoint>,
                           fusion_pair_comment_map<navitia::type::Line>,
                           fusion_pair_comment_map<navitia::type::Route>,
                           fusion_pair_comment_map<navitia::type::VehicleJourney>,
                           fusion_pair_comment_map<navitia::type::LineGroup>,
                           boost::fusion::pair<navitia::type::StopTime, std::map<stop_time_key, comment_list>>>;

    comment_map_type map;
};

}  // namespace type
}  // namespace navitia
