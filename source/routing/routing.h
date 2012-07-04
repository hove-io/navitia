#pragma once

#include "type/pt_data.h"

namespace navitia { namespace routing {
using type::idx_t;

/** Étape d'un itinéraire*/
struct PathItem{
    std::string stop_point_name;
    int time;
    int day;
};

/** Un itinéraire complet */
struct Path {
    int duration;
    int nb_changes;
    int percent_visited;
    std::vector<PathItem> items;
};

/** Classe abstraite que tous les calculateurs doivent implémenter */
struct AbstractRouter {
    virtual Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) = 0;
};
}}
