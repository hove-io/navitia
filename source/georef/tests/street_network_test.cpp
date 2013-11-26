#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_street_network
#include <boost/test/unit_test.hpp>
#include "georef/georef.h"
#include "builder.h"
#include "type/data.h"

//to be able to test private member.
#define private public
#include"georef/street_network.h"
#undef private

using namespace navitia::georef;
using namespace navitia::streetnetwork;

using namespace navitia;

struct computation_results {
    double distance; //asked distance
    std::vector<float> distances_matrix; //distances matrix
    std::vector<vertex_t> predecessor;

    computation_results(double d, const StreetNetwork& worker) : distance(d), distances_matrix(worker.distances), predecessor(worker.predecessors) {}

    bool operator ==(const computation_results& other) {

        BOOST_REQUIRE_CLOSE(other.distance, distance, 0.001);

        BOOST_REQUIRE_EQUAL(other.distances_matrix.size(), distances_matrix.size());

        for (size_t i = 0 ; i < distances_matrix.size() ; ++i) {
            BOOST_REQUIRE_CLOSE(other.distances_matrix.at(i), distances_matrix.at(i), 0.001);
        }

        BOOST_REQUIRE(predecessor == other.predecessor);

        return true;
    }
};

std::string get_name(int i, int j) { return std::string(i + "_" + j); }
bool almost_equal(float a, float b) {
    return fabs(a - b) < 0.00001;
}

/**
  * The aim of the test is to check that the street network answer give the same answer
  * to multiple get_distance question
  *
  **/
BOOST_AUTO_TEST_CASE(idempotence) {
    //graph creation
    type::Data data;
    GeoRef geo_ref;
    GraphBuilder b(geo_ref);
    size_t square_size(10);

    //we build a dumb square graph
    for (size_t i = 0; i < square_size ; ++i) {
        for (size_t j = 0; j < square_size ; ++j) {
            std::string name(get_name(i, j));
            b(name, i, j);
        }
    }
    for (size_t i = 0; i < square_size - 1; ++i) {
        for (size_t j = 0; j < square_size - 1; ++j) {
            std::string name(get_name(i, j));

            //we add edge to the next vertex (the value is not important)
            b.add_edge(name, get_name(i, j + 1), (i + j) * j);
            b.add_edge(name, get_name(i + 1, j), (i + j) * i);
        }
    }

    StreetNetwork worker(geo_ref);

    //we project 2 stations
    type::GeographicalCoord start;
    start.set_xy(2., 2.);

    type::StopPoint* sp = new type::StopPoint();
    sp->coord.set_xy(8., 8.);
    sp->idx = 0;
    data.pt_data.stop_points.push_back(sp);
    geo_ref.project_stop_points(data.pt_data.stop_points);

    const GeoRef::ProjectionByMode& projections = geo_ref.projected_stop_points[sp->idx];
    const ProjectionData proj = projections[type::Mode_e::Walking];

    BOOST_REQUIRE(proj.found); //we have to be able to project this point (on the walking graph)

    geo_ref.build_proximity_list();

    type::idx_t target_idx(sp->idx);

    const bool use_second = false;

    double distance = worker.get_distance(start, target_idx, use_second, type::Mode_e::Walking, false);

    //we have to find a way to get there
    BOOST_REQUIRE_NE(distance, std::numeric_limits<float>::max());

    std::cout << "distance " << distance
              << " proj distance to source " << proj.source_distance
              << " proj distance to target " << proj.target_distance
            << " distance to source " << worker.distances[proj.source]
                 << " distance to target " << worker.distances[proj.target] << std::endl;

    // the distance matrix also has to be updated
    BOOST_REQUIRE(almost_equal(worker.distances[proj.source] + proj.source_distance, distance) //we have to take into account the projection distance
            || almost_equal(worker.distances[proj.target] + proj.target_distance, distance));

    computation_results first_res {distance, worker};

    //we ask again with the init again
    {
        double other_distance = worker.get_distance(start, target_idx, use_second, type::Mode_e::Walking, false);

        computation_results other_res {other_distance, worker};

        //we have to find a way to get there
        BOOST_REQUIRE_NE(other_distance, std::numeric_limits<float>::max());
        // the distance matrix  also has to be updated
        BOOST_REQUIRE(almost_equal(worker.distances[proj.source] + proj.source_distance, other_distance)
                || almost_equal(worker.distances[proj.target] + proj.target_distance, other_distance));

        BOOST_REQUIRE(first_res == other_res);
    }

    //we ask again without a init
    {
        double other_distance = worker.get_distance(start, target_idx, use_second, type::Mode_e::Walking, true);

        computation_results other_res {other_distance, worker};
        //we have to find a way to get there
        BOOST_CHECK_NE(other_distance, std::numeric_limits<float>::max());

        BOOST_REQUIRE(almost_equal(worker.distances[proj.source] + proj.source_distance, other_distance)
                || almost_equal(worker.distances[proj.target] + proj.target_distance, other_distance));

        BOOST_CHECK(first_res == other_res);
    }
}
