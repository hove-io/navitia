#pragma once
#include "third_party/osmpbfreader/osmpbfreader.h"
#include "ed/types.h"
#include <unordered_map>

namespace navitia { namespace  connectors {
    ed::types::Poi fill_poi(const uint64_t osm_id, const double lon, const double lat,
            const CanalTP::Tags & tags, const size_t potential_id,
            std::unordered_map<std::string, ed::types::PoiType>& poi_types);
}}
