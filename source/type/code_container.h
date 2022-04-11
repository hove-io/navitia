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

#include "utils/functions.h"
#include "utils/serialization_fusion_map.h"

#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm.hpp>

#include <type_traits>
#include <map>
#include <vector>

namespace navitia {
namespace type {

struct StopArea;
struct Network;
struct Company;
struct Line;
struct Route;
struct VehicleJourney;
struct StopPoint;
struct Calendar;

/**
 * Code container
 *
 * PT objects can contain different codes (from different source,
 * typically).  For example, stop_areas can have UIC codes.  This
 * container manage, for a subset of pt_types, these codes.  You can
 * get all the codes of a given object, or searching objects of a
 * given pt_type for a code type and its value (for example, the
 * stop_areas that have the UIC code '8727100')
 */
struct CodeContainer {
    using Codes = std::map<std::string, std::vector<std::string>>;

    template <typename T>
    using Objs = std::vector<const T*>;

    using SupportedTypes =
        boost::mpl::vector<StopArea, Network, Company, Line, Route, VehicleJourney, StopPoint, Calendar>;

    template <typename T>
    const Codes& get_codes(const T* obj) const {
        const auto& m = at_key<T>(obj_map);
        return find_or_default(obj, m);
    }

    template <typename T>
    const Objs<T>& get_objs(const std::string& key, const std::string& val) const {
        const auto& m = at_key<T>(code_map);
        return find_or_default({key, val}, m);
    }

    template <typename T>
    void add(const T* obj, const std::string& key, const std::string& val) {
        assert(obj);
        auto& obj_m = at_key<T>(obj_map);
        obj_m[obj][key].push_back(val);

        auto& code_m = at_key<T>(code_map);
        code_m[{key, val}].push_back(obj);
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& obj_map& code_map;
    }

private:
    template <typename T>
    using CodesByObjMap = std::map<const T*, Codes>;

    template <typename T>
    struct MakePairByObj {
        using type = typename boost::fusion::result_of::make_pair<T, CodesByObjMap<T>>::type;
    };
    using ObjMap = typename boost::fusion::result_of::as_map<
        typename boost::mpl::transform<SupportedTypes, MakePairByObj<boost::mpl::_1>>::type>::type;

    template <typename T>
    using ObjsByCodeMap = std::map<std::pair<std::string, std::string>, Objs<T>>;

    template <typename T>
    struct MakePairByCode {
        using type = typename boost::fusion::result_of::make_pair<T, ObjsByCodeMap<T>>::type;
    };
    using CodeMap = typename boost::fusion::result_of::as_map<
        typename boost::mpl::transform<SupportedTypes, MakePairByCode<boost::mpl::_1>>::type>::type;

    template <typename T>
    using RemoveConstPtr = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

    template <typename T, typename Sequence>
    static auto at_key(Sequence& seq) -> decltype(boost::fusion::at_key<RemoveConstPtr<T>>(seq)) {
        return boost::fusion::at_key<RemoveConstPtr<T>>(seq);
    }

    ObjMap obj_map;
    CodeMap code_map;
};

}  // namespace type
}  // namespace navitia
