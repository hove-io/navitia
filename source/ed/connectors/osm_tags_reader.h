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

#include <bitset>
#include <map>
#include <osmpbfreader/osmpbfreader.h>
#include "ed/types.h"

namespace ed {
namespace connectors {

constexpr uint8_t CYCLE_FWD = 0;
constexpr uint8_t CYCLE_BWD = 1;
constexpr uint8_t CAR_FWD = 2;
constexpr uint8_t CAR_BWD = 3;
constexpr uint8_t FOOT_FWD = 4;
constexpr uint8_t FOOT_BWD = 5;
constexpr uint8_t VISIBLE = 6;

class SpeedsParser;

/// return properties on modes and directions that are possible on a way
std::bitset<8> parse_way_tags(const std::map<std::string, std::string>& tags);

boost::optional<float> parse_way_speed(const std::map<std::string, std::string>& tags,
                                       const SpeedsParser& default_speed);

ed::types::Poi fill_poi(const uint64_t osm_id,
                        const double lon,
                        const double lat,
                        const Hove::Tags& tags,
                        const size_t potential_id,
                        std::unordered_map<std::string, ed::types::PoiType>& poi_types);

struct RuleOsmTag2PoiType {
    std::string poi_type_id;
    std::map<std::string, std::string> osm_tag_filters;
};

struct PoiTypeParams {
    std::map<std::string, std::string> poi_types;
    std::vector<RuleOsmTag2PoiType> rules;

    PoiTypeParams(const std::string& json_params);
    const RuleOsmTag2PoiType* get_applicable_poi_rule(const Hove::Tags& tags) const;
};

const std::string get_postal_code_from_tags(const Hove::Tags& tags);

}  // namespace connectors
}  // namespace ed
