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

#include "route_schedules.h"

#include "ptreferential/ptreferential.h"
#include "request_handle.h"
#include "routing/dataraptor.h"
#include "thermometer.h"
#include "type/datetime.h"
#include "type/pb_converter.h"
#include "utils/paginate.h"

#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/for_each.hpp>

namespace pt = boost::posix_time;

namespace navitia {
namespace timetables {

std::vector<std::vector<routing::datetime_stop_time>> get_all_route_stop_times(
    const nt::Route* route,
    const DateTime& date_time,
    const DateTime& max_datetime,
    const size_t max_stop_date_times,
    const type::Data& d,
    const type::RTLevel rt_level,
    const boost::optional<const std::string>& calendar_id) {
    std::vector<std::vector<routing::datetime_stop_time>> result;

    const auto& journey_patterns = d.dataRaptor->jp_container.get_jps_from_route()[routing::RouteIdx(*route)];

    // We take the first journey pattern points from every journey_pattern
    std::vector<routing::JppIdx> first_journey_pattern_points;
    for (const auto& jp_idx : journey_patterns) {
        const auto& jp = d.dataRaptor->jp_container.get(jp_idx);
        if (jp.jpps.empty()) {
            continue;
        }
        first_journey_pattern_points.push_back(jp.jpps.front());
    }

    std::vector<routing::datetime_stop_time> first_dt_st;
    if (!calendar_id) {
        // If there is no calendar we get all stop times in
        // the desired timeframe
        first_dt_st = get_stop_times(routing::StopEvent::pick_up, first_journey_pattern_points, date_time, max_datetime,
                                     max_stop_date_times, d, rt_level);
    } else {
        // Otherwise we only take stop_times in a vehicle_journey associated to
        // the desired calendar and in the timeframe
        first_dt_st = get_calendar_stop_times(first_journey_pattern_points, DateTimeUtils::hour(date_time),
                                              DateTimeUtils::hour(max_datetime), d, *calendar_id);
    }

    // we need to load the next datetimes for each jp
    for (const auto& ho : first_dt_st) {
        result.emplace_back();
        DateTime dt = ho.first;
        for (const type::StopTime& stop_time : ho.second->vehicle_journey->stop_time_list) {
            if (!stop_time.is_frequency()) {
                if (!calendar_id) {
                    dt = DateTimeUtils::shift(dt, stop_time.departure_time);
                } else {
                    // for calendar, we need to shift the time to local time
                    const auto utc_to_local_offset = stop_time.vehicle_journey->utc_to_local_offset();
                    // we don't care about the date and about the overmidnight cases
                    dt = DateTimeUtils::hour(stop_time.departure_time + utc_to_local_offset);
                }
            } else {
                // for frequencies, we only need to add the stoptime offset to the first stoptime
                dt = ho.first + (stop_time.departure_time - ho.second->departure_time);
                // NOTE: for calendar, we don't need to add again the utc offset, since for frequency
                // vj all stop_times are relative to the start of the vj (and the UTC offset has  already
                // been included in the first stop time)
            }
            result.back().push_back(std::make_pair(dt, &stop_time));
        }
    }

    if (calendar_id) {
        // for calendar's schedule we need to sort the datetimes in a different way
        // we add 24h for each vj's dt that starts before 'date_time' so the vj will be sorted
        // at the end (with the complex score of the route schedule)
        for (auto& vjs_stops : result) {
            bool add_day = false;
            for (auto& dt_st : vjs_stops) {
                // If a stop time is before limit then add a day from here to end of stops
                if (add_day || (DateTimeUtils::hour(dt_st.first) < DateTimeUtils::hour(date_time))) {
                    dt_st.first += DateTimeUtils::SECONDS_PER_DAY;
                    add_day = true;
                }
            }
        }
    }
    return result;
}

namespace {
// The score is the number of aligned cells that are not on order
// minus the number of aligned cells that are on order. Ex:
// v1 v2
//  1  2 => -1
//  2  3 => -1
//  3  4 => -1
//  5  5
//  7
//  9  7 =>  1
// score:   -2
int score(const std::vector<routing::datetime_stop_time>& v1, const std::vector<routing::datetime_stop_time>& v2) {
    int res = 0;
    boost::range::for_each(v1, v2, [&](const routing::datetime_stop_time& a, const routing::datetime_stop_time& b) {
        if (a.second == nullptr || b.second == nullptr) {
            return;
        }
        if (a.first < b.first) {
            --res;
        } else if (a.first > b.first) {
            ++res;
        }
    });
    return res;
}
using Graph = boost::adjacency_matrix<boost::directedS>;
struct Edge {
    uint32_t source;
    uint32_t target;
    uint32_t score;

    friend bool operator<(const Edge& a, const Edge& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        if (a.source != b.source) {
            return a.source < b.source;
        }
        return a.target < b.target;
    }
};
std::vector<Edge> create_edges(std::vector<std::vector<routing::datetime_stop_time>>& v) {
    std::vector<Edge> edges;
    for (uint32_t i = 0; i < v.size(); ++i) {
        for (uint32_t j = i + 1; j < v.size(); ++j) {
            const int s = score(v[i], v[j]);
            if (s > 0) {
                edges.push_back({i, j, uint32_t(s)});
            } else if (s < 0) {
                edges.push_back({j, i, uint32_t(-s)});
            }
        }
    }
    boost::sort(edges);
    return edges;
}
struct IsDag {
    bool operator()(const Graph& g) {
        ++nb_call;
        order.reserve(num_vertices(g));
        try {
            order.clear();
            boost::topological_sort(g, std::back_inserter(order));
            return true;
        } catch (boost::not_a_dag&) {
            return false;
        }
    }
    std::vector<uint32_t> order;
    size_t nb_call = 0;
};
// Using http://en.wikipedia.org/wiki/Ranked_pairs to sort the vj.  As
// if each stop time vote according to the time of the vj at its stop
// time (don't care for the vj that don't stop).
std::vector<uint32_t> compute_order(const size_t nb_vertices, const std::vector<Edge>& edges) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    Graph g(nb_vertices);
    IsDag is_dag;

    LOG4CPLUS_DEBUG(logger, "trying total topological sort with nb_vertices = " << nb_vertices);
    // most of the time, the graph will not have any cycle, thus we
    // test directly a topological sort.
    for (const auto& edge : edges) {
        add_edge(edge.source, edge.target, g);
    }
    if (is_dag(g)) {
        LOG4CPLUS_DEBUG(logger, "total topological sort is working, great!");
        return std::move(is_dag.order);
    }

    // there is a cycle, thus we run the complete algorithm.
    g = Graph(nb_vertices);  // remove all edges
    size_t nb_removed_edges = 0;
    LOG4CPLUS_DEBUG(logger, "trying total ranked pair with nb_edges = " << edges.size());

    // Straintforward implementation of ranked pair is:
    //
    // for (const auto& edge: edges) {
    //    const auto e_desc = add_edge(edge.source, edge.target, g).first;
    //    if (! is_dag(g)) { ++nb_removed_edges; remove_edge(e_desc, g); }
    //}
    //
    // Let n be the number of vertices, and e be the number of
    // edges. O(e) = O(n²).  The straintforward implementation is
    //
    //   O(e * (n + e)) = O(e²) = O(n⁴)
    //
    // In practice, this algorithm is too costy for big route
    // schedules.  Let k be the number of edges that will be removed.
    // O(k) = O(n²).  In practice, it seems that Omega(k) = Omega(n).
    // If we do binary search to find each edges that we need to
    // remove, the complexity is
    //
    //   O(k * log(e) * (n + e)) = O(k * n² * log(n²)) = O(k * n² * log(n))
    //
    // Thus, in our case, this algorithm seems to run in
    //
    //   Omega(k * n² * log(n)) = Omega(n³ * log(n))
    //
    // which is much better than the naive implementation.  In
    // practice, our problematic case run in 33s using the naive
    // algorithm, and 1s using the binary search.
    //
    size_t done_until = 0;
    // the edges in the graph that we may remove during the current binary search
    std::vector<Graph::edge_descriptor> e_descrs;
    e_descrs.reserve(edges.size());
    while (done_until < edges.size()) {  // while not all edges are done
        // binary searching the next edge that create a cycle,
        // inspired by
        // https://en.wikipedia.org/wiki/Binary_search_algorithm#Deferred_detection_of_equality
        size_t imin = done_until;
        size_t imax = edges.size();
        while (imin < imax) {
            const size_t imid = (imin + imax) / 2;

            // modifying the graph to be in the wanted state
            const size_t nb_elt_to_add = imid - done_until + 1;
            if (e_descrs.size() < nb_elt_to_add) {
                // add corresponding edges
                while (nb_elt_to_add != e_descrs.size()) {
                    const size_t idx = done_until + e_descrs.size();
                    e_descrs.push_back(add_edge(edges[idx].source, edges[idx].target, g).first);
                }
            } else {
                // remove corresponding edges
                while (nb_elt_to_add != e_descrs.size()) {
                    remove_edge(e_descrs.back(), g);
                    e_descrs.pop_back();
                }
            }

            if (is_dag(g)) {
                imin = imid + 1;
            } else {
                imax = imid;
            }
        }

        // We've found the problematic edge (or we are at the end).
        assert(imin == imax);
        if (imax < edges.size()) {
            // remove the edge that create the cycle
            ++nb_removed_edges;
            remove_edge(e_descrs.back(), g);
        }
        // current state of the graph is valid until imax + 1
        done_until = imax + 1;
        e_descrs.clear();
    }
    // Great, we have the graph, end of the binary search
    // implementation of ranked pair.

    if (!is_dag(g)) {
        assert(false);
    }  // to compute the order on the final graph
    LOG4CPLUS_DEBUG(logger, "total ranked pair done with nb_removed_edges = " << nb_removed_edges
                                                                              << ", nb_topo_sort = " << is_dag.nb_call);
    return std::move(is_dag.order);
}
void ranked_pairs_sort(std::vector<std::vector<routing::datetime_stop_time>>& v) {
    const auto edges = create_edges(v);
    const auto order = compute_order(v.size(), edges);

    // reordering v according to the given order
    std::vector<std::vector<routing::datetime_stop_time>> res;
    res.reserve(v.size());
    for (const auto& idx : order) {
        res.push_back(std::move(v[idx]));
    }
    boost::swap(res, v);
}
}  // namespace

static std::vector<std::vector<routing::datetime_stop_time>> make_matrix(
    const std::vector<std::vector<routing::datetime_stop_time>>& stop_times,
    const Thermometer& thermometer) {
    // result group stop_times by stop_point, tmp by vj.
    const size_t thermometer_size = thermometer.get_thermometer().size();
    std::vector<std::vector<routing::datetime_stop_time>> result(
        thermometer_size, std::vector<routing::datetime_stop_time>(stop_times.size())),
        tmp(stop_times.size(), std::vector<routing::datetime_stop_time>(thermometer_size));
    // We match every stop_time with the journey pattern
    int y = 0;
    for (const auto& vec : stop_times) {
        const auto* vj = vec.front().second->vehicle_journey;
        std::vector<uint32_t> orders = thermometer.stop_times_order(*vj);
        int order = 0;
        for (const auto& dt_stop_time : vec) {
            tmp.at(y).at(orders.at(order)) = dt_stop_time;
            ++order;
        }
        ++y;
    }

    ranked_pairs_sort(tmp);
    // We rotate the matrice, so it can be handle more easily in route_schedule
    for (size_t i = 0; i < tmp.size(); ++i) {
        for (size_t j = 0; j < tmp[i].size(); ++j) {
            result[j][i] = tmp[i][j];
        }
    }
    return result;
}

void route_schedule(PbCreator& pb_creator,
                    const std::string& filter,
                    const boost::optional<const std::string>& calendar_id,
                    const std::vector<std::string>& forbidden_uris,
                    const pt::ptime datetime,
                    uint32_t duration,
                    size_t max_stop_date_times,
                    const uint32_t max_depth,
                    int count,
                    int start_page,
                    const type::RTLevel rt_level) {
    RequestHandle handler(pb_creator, datetime, duration, calendar_id);

    if (pb_creator.has_error()) {
        return;
    }

    auto pt_datetime = to_posix_time(handler.date_time, *pb_creator.data);
    auto pt_max_datetime = to_posix_time(handler.max_datetime, *pb_creator.data);
    pb_creator.action_period = pt::time_period(pt_datetime, pt_max_datetime);

    Thermometer thermometer;
    type::Indexes routes_idx;
    try {
        routes_idx = ptref::make_query(type::Type_e::Route, filter, forbidden_uris, *pb_creator.data);
    } catch (const ptref::parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse filter" + parse_error.more);
        return;
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + ptref_error.more);
        return;
    }
    size_t total_result = routes_idx.size();
    routes_idx = paginate(routes_idx, count, start_page);
    for (const auto& route_idx : routes_idx) {
        auto route = pb_creator.data->pt_data->routes[route_idx];
        auto stop_times = get_all_route_stop_times(route, handler.date_time, handler.max_datetime, max_stop_date_times,
                                                   *pb_creator.data, rt_level, calendar_id);
        const auto& jps = pb_creator.data->dataRaptor->jp_container.get_jps_from_route()[routing::RouteIdx(*route)];
        std::vector<vector_idx> stop_points;
        for (const auto& jp_idx : jps) {
            const auto& jp = pb_creator.data->dataRaptor->jp_container.get(jp_idx);
            stop_points.emplace_back();
            for (const auto& jpp_idx : jp.jpps) {
                const auto& jpp = pb_creator.data->dataRaptor->jp_container.get(jpp_idx);
                stop_points.back().push_back(jpp.sp_idx.val);
            }
        }
        thermometer.generate_thermometer(stop_points);
        auto matrix = make_matrix(stop_times, thermometer);

        auto schedule = pb_creator.add_route_schedules();
        pbnavitia::Table* table = schedule->mutable_table();
        auto m_pt_display_informations = schedule->mutable_pt_display_informations();
        pb_creator.fill(route, m_pt_display_informations, 0);

        std::vector<bool> is_vj_set(stop_times.size(), false);
        for (size_t i = 0; i < stop_times.size(); ++i) {
            table->add_headers();
        }
        for (unsigned int i = 0; i < thermometer.get_thermometer().size(); ++i) {
            type::idx_t spidx = thermometer.get_thermometer()[i];
            const type::StopPoint* sp = pb_creator.data->pt_data->stop_points[spidx];
            pbnavitia::RouteScheduleRow* row = table->add_rows();
            pb_creator.fill(sp, row->mutable_stop_point(), max_depth);

            for (unsigned int j = 0; j < stop_times.size(); ++j) {
                const auto& dt_stop_time = matrix[i][j];
                if (!is_vj_set[j] && dt_stop_time.second != nullptr) {
                    pbnavitia::Header* header = table->mutable_headers(j);
                    pbnavitia::PtDisplayInfo* vj_display_information = header->mutable_pt_display_informations();
                    auto vj = dt_stop_time.second->vehicle_journey;
                    auto sts = std::vector<const nt::StopTime*>{dt_stop_time.second};
                    const auto& vj_st = navitia::VjStopTimes(vj, sts);
                    pb_creator.fill(&vj_st, vj_display_information, 0);
                    vj_display_information->set_headsign(pb_creator.data->pt_data->headsign_handler.get_headsign(vj));
                    for (const auto& headsign : pb_creator.data->pt_data->headsign_handler.get_all_headsigns(vj)) {
                        vj_display_information->add_headsigns(headsign);
                    }
                    pb_creator.fill_additional_informations(header->mutable_additional_informations(),
                                                            vj->has_datetime_estimated(), vj->has_odt(),
                                                            vj->has_zonal_stop_point());
                    is_vj_set[j] = true;
                }

                auto pb_dt = row->add_date_times();
                const auto& st_calendar =
                    navitia::StopTimeCalendar(dt_stop_time.second, dt_stop_time.first, calendar_id);
                pb_creator.fill(&st_calendar, pb_dt, max_depth);
            }
        }
        if (!pb_creator.disable_geojson) {
            pb_creator.fill(&route->shape, schedule->mutable_geojson(), 0);
        }

        // Add additiona_informations in each route:
        if (stop_times.empty() && (rt_level != type::RTLevel::Base)) {
            auto tmp_stop_times =
                get_all_route_stop_times(route, handler.date_time, handler.max_datetime, max_stop_date_times,
                                         *pb_creator.data, type::RTLevel::Base, calendar_id);
            if (!tmp_stop_times.empty()) {
                schedule->set_response_status(pbnavitia::ResponseStatus::active_disruption);
            } else {
                schedule->set_response_status(pbnavitia::ResponseStatus::no_departure_this_day);
            }
        }
    }
    pb_creator.make_paginate(total_result, start_page, count, pb_creator.route_schedules_size());
}
}  // namespace timetables
}  // namespace navitia
