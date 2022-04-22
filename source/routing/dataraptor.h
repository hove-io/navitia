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
#include "type/pt_data.h"
#include "type/datetime.h"
#include "routing/raptor_utils.h"
#include "utils/idx_map.h"
#include "routing/next_stop_time.h"
#include "routing/journey_pattern_container.h"
#include "routing/labels.h"

#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>

namespace navitia {
namespace routing {

/** Données statiques qui ne sont pas modifiées pendant le calcul */
struct dataRAPTOR {
    // cache friendly access to the connections
    struct Connections {
        struct Connection {
            DateTime duration;          // walking_duration + margin duration to be able to catch the next vehicle
            DateTime walking_duration;  // walking_duration, filled with StopPointConnection.display_duration
            SpIdx sp_idx;
        };
        void load(const navitia::type::PT_Data&);

        // for a stop point, get the corresponding forward connections
        IdxMap<type::StopPoint, std::vector<Connection>> forward_connections;
        // for a stop point, get the corresponding backward connections
        IdxMap<type::StopPoint, std::vector<Connection>> backward_connections;
    };
    Connections connections;
    DateTime min_connection_time;

    // cache friendly access to JourneyPatternPoints from a StopPoint
    struct JppsFromSp {
        // compressed JourneyPatternPoint
        struct Jpp {
            JppIdx idx;      // index
            JpIdx jp_idx;    // corresponding JourneyPattern index
            uint16_t order;  // order of the jpp in its jp
        };
        inline const std::vector<Jpp>& operator[](const SpIdx& sp) const { return jpps_from_sp[sp]; }
        void load(const type::PT_Data&, const JourneyPatternContainer&);
        void filter_jpps(const boost::dynamic_bitset<>& valid_jpps);

        inline IdxMap<type::StopPoint, std::vector<Jpp>>::const_iterator begin() const { return jpps_from_sp.begin(); }
        inline IdxMap<type::StopPoint, std::vector<Jpp>>::const_iterator end() const { return jpps_from_sp.end(); }

    private:
        IdxMap<type::StopPoint, std::vector<Jpp>> jpps_from_sp;
    };
    JppsFromSp jpps_from_sp;

    // cache friendly access to in order JourneyPatternPoints from a JourneyPattern
    struct JppsFromJp {
        // compressed JourneyPatternPoint
        struct Jpp {
            JppIdx idx;
            SpIdx sp_idx;
            bool has_freq;
        };
        inline const std::vector<Jpp>& operator[](const JpIdx& jp) const { return jpps_from_jp[jp]; }
        void load(const JourneyPatternContainer&);

    private:
        IdxMap<JourneyPattern, std::vector<Jpp>> jpps_from_jp;
    };
    JppsFromJp jpps_from_jp;

    NextStopTimeData next_stop_time_data;
    std::unique_ptr<CachedNextStopTimeManager> cached_next_st_manager;

    JourneyPatternContainer jp_container;

    // blank labels, to fast init labels with a memcpy
    Labels labels_const;
    Labels labels_const_reverse;

    // jp_validity_patterns[date][jp_idx] == any(vj.validity_pattern->check2(date) for vj in jp)
    flat_enum_map<type::RTLevel, std::vector<boost::dynamic_bitset<>>> jp_validity_patterns;

    dataRAPTOR() = default;
    void load(const navitia::type::PT_Data&, size_t cache_size = 10);

    void warmup(const dataRAPTOR& other);
};

}  // namespace routing
}  // namespace navitia
