#pragma once
#include "ptreferential/ptreferential.h"
#include "routing/raptor.h"
#include "routing/routing.h"
#include "time_tables/next_departures.h"
#include "type/pb_converter.h"

#include <unordered_map>

namespace navitia { namespace timetables {

typedef std::pair<dt_st, dt_st> pair_dt_st;


std::unordered_map<type::idx_t, uint32_t> get_arrival_order(const std::vector<type::idx_t> &departure_routepoint, const std::string &arrival_filter, type::Data & data);

std::vector<pair_dt_st> departure_board(const std::string &departure_filter, const std::string &arrival_filter,
                                        const routing::DateTime &datetime, const routing::DateTime &max_datetime, const int nb_departures,
                                        type::Data & data, routing::raptor::RAPTOR &raptor);

pbnavitia::Response departure_board(const std::string &departure_filter, const std::string &arrival_filter,
                                    const std::string &str_dt, const std::string &str_max_dt,
                                    const int nb_departures, const int depth, type::Data & data, routing::raptor::RAPTOR &raptor);

}}
