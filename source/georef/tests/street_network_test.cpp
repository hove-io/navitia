/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_street_network
#include "georef/georef.h"
#include "builder.h"
#include "type/data.h"
#include "type/pt_data.h"

#include "georef/street_network.h"
#include <boost/test/unit_test.hpp>

using namespace navitia::georef;

using namespace navitia;
namespace bt = boost::posix_time;

using dir = ProjectionData::Direction;

struct computation_results {
    navitia::time_duration duration;                       // asked duration
    std::vector<navitia::time_duration> durations_matrix;  // duration matrix
    std::vector<vertex_t> predecessor;

    computation_results(navitia::time_duration d, const PathFinder& worker)
        : duration(d), durations_matrix(worker.distances), predecessor(worker.predecessors) {}

    bool operator==(const computation_results& other) {
        BOOST_CHECK_EQUAL(other.duration, duration);

        BOOST_REQUIRE_EQUAL(other.durations_matrix.size(), durations_matrix.size());

        for (size_t i = 0; i < durations_matrix.size(); ++i) {
            BOOST_CHECK_EQUAL(other.durations_matrix.at(i), durations_matrix.at(i));
        }

        BOOST_CHECK(predecessor == other.predecessor);

        return true;
    }
};

static std::string get_name(int i, int j) {
    std::stringstream ss;
    ss << i << "_" << j;
    return ss.str();
}

/**
 * The aim of the test is to check that the street network answer give the same answer
 * to multiple get_distance question
 *
 **/
BOOST_AUTO_TEST_CASE(idempotence) {
    // graph creation
    type::Data data;
    GraphBuilder b;
    size_t square_size(10);

    // we build a dumb square graph
    for (size_t i = 0; i < square_size; ++i) {
        for (size_t j = 0; j < square_size; ++j) {
            std::string name(get_name(i, j));
            b(name, i, j);
        }
    }
    for (size_t i = 0; i < square_size - 1; ++i) {
        for (size_t j = 0; j < square_size - 1; ++j) {
            std::string name(get_name(i, j));

            // we add edge to the next vertex (the value is not important)
            b.add_edge(name, get_name(i, j + 1), navitia::seconds((i + j) * j));
            b.add_edge(name, get_name(i + 1, j), navitia::seconds((i + j) * i));
        }
    }

    PathFinder worker(b.geo_ref);

    // we project 2 stations
    type::GeographicalCoord start;
    start.set_xy(2., 2.);

    type::StopPoint* sp = new type::StopPoint();
    sp->coord.set_xy(8., 8.);
    sp->idx = 0;
    data.pt_data->stop_points.push_back(sp);
    b.geo_ref.init();
    b.geo_ref.project_stop_points(data.pt_data->stop_points);

    const GeoRef::ProjectionByMode& projections = b.geo_ref.projected_stop_points[sp->idx];
    const ProjectionData proj = projections[type::Mode_e::Walking];

    BOOST_REQUIRE(proj.found);  // we have to be able to project this point (on the walking graph)

    b.geo_ref.build_proximity_list();

    type::idx_t target_idx(sp->idx);

    worker.init(start, type::Mode_e::Walking, georef::default_speed[type::Mode_e::Walking]);

    auto distance = worker.get_distance(target_idx);

    // we have to find a way to get there
    BOOST_REQUIRE_NE(distance, bt::pos_infin);

    std::cout << "distance " << distance << " proj distance to source " << proj.distances[dir::Source]
              << " proj distance to target " << proj.distances[dir::Target] << " distance to source "
              << worker.distances[proj[dir::Source]] << " distance to target " << worker.distances[proj[dir::Target]]
              << std::endl;

    // the distance matrix also has to be updated
    BOOST_CHECK(
        worker.distances[proj[dir::Source]]
                + navitia::seconds(proj.distances[dir::Source] / double(default_speed[type::Mode_e::Walking]))
            == distance  // we have to take into account the projection distance
        || worker.distances[proj[dir::Target]]
                   + navitia::seconds(proj.distances[dir::Target] / double(default_speed[type::Mode_e::Walking]))
               == distance);

    computation_results first_res{distance, worker};

    // we ask again with the init again
    {
        worker.init(start, type::Mode_e::Walking, georef::default_speed[type::Mode_e::Walking]);
        auto other_distance = worker.get_distance(target_idx);

        computation_results other_res{other_distance, worker};

        // we have to find a way to get there
        BOOST_REQUIRE_NE(other_distance, bt::pos_infin);
        // the distance matrix  also has to be updated
        BOOST_CHECK(
            worker.distances[proj[dir::Source]]
                    + navitia::seconds(proj.distances[dir::Source] / double(default_speed[type::Mode_e::Walking]))
                == other_distance
            || worker.distances[proj[dir::Target]]
                       + navitia::seconds(proj.distances[dir::Target] / double(default_speed[type::Mode_e::Walking]))
                   == other_distance);

        BOOST_REQUIRE(first_res == other_res);
    }

    // we ask again without a init
    {
        auto other_distance = worker.get_distance(target_idx);

        computation_results other_res{other_distance, worker};
        // we have to find a way to get there
        BOOST_CHECK_NE(other_distance, bt::pos_infin);

        BOOST_CHECK(
            worker.distances[proj[dir::Source]]
                    + navitia::seconds(proj.distances[dir::Source] / double(default_speed[type::Mode_e::Walking]))
                == other_distance
            || worker.distances[proj[dir::Target]]
                       + navitia::seconds(proj.distances[dir::Target] / double(default_speed[type::Mode_e::Walking]))
                   == other_distance);

        BOOST_CHECK(first_res == other_res);
    }
}
