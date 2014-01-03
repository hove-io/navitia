#pragma once
#include "georef.h"
#include <boost/graph/filtered_graph.hpp>

namespace ng = navitia::georef;
namespace bt = boost::posix_time;

namespace navitia { namespace streetnetwork {

struct SpeedDistanceCombiner : public std::binary_function<bt::time_duration, bt::time_duration, bt::time_duration> {
    /// speed factor compared to the default speed of the transportation mode
    /// speed_factor = 2 means the speed is twice the default speed of the given transportation mode
    float speed_factor;
    SpeedDistanceCombiner(float speed_) : speed_factor(speed_) {}
    inline bt::time_duration operator()(bt::time_duration a, bt::time_duration b) const {
        if (a == bt::pos_infin || b == bt::pos_infin)
            return bt::pos_infin;
        return a + divide_by_speed(b);
    }

    //needs to redefine the / operator since boost only define integer division
    inline bt::time_duration divide_by_speed(bt::time_duration t) const {
        return bt::milliseconds(t.total_milliseconds() / speed_factor);
    }
};
template <typename T>
using map_by_mode = flat_enum_map<type::Mode_e, T>;
/**
 * create a map of map of boolean from a map of map of allowed mode
 * (because it's simpler to define with only the allowed mode, but  more efficient with the boolean masks)
 */
inline map_by_mode<map_by_mode<bool>> create_from_allowedlist(map_by_mode<std::vector<nt::Mode_e>> allowed_modes) {
    map_by_mode<map_by_mode<bool>> res;
    for (auto modes_pair : allowed_modes) {
        res[modes_pair.first] = {}; //force false initialization of all members
        for (auto mode : modes_pair.second) {
            res[modes_pair.first][mode] = true;
        }
    }
    return res;
}

const auto allowed_transportation_mode = create_from_allowedlist({{{
                                                                {type::Mode_e::Walking}, //for walking, only walking is allowed
                                                                {type::Mode_e::Bike}, //for biking, only bike
                                                                {type::Mode_e::Walking, type::Mode_e::Car}, //for car, walking and car is allowed
                                                                {type::Mode_e::Walking, type::Mode_e::Vls} //for vls, walking and vls is allowed
                                                          }}});

struct TransportationModeFilter {
    flat_enum_map<type::Mode_e, bool> acceptable_modes; //map associating a boolean to a mode,
    type::idx_t nb_edge_by_mode;
    TransportationModeFilter() = default;
    TransportationModeFilter(type::Mode_e mode, const georef::GeoRef& geo_ref) :
        acceptable_modes(allowed_transportation_mode[mode]),
        nb_edge_by_mode(geo_ref.offsets[1]) //the second elt in the offset array is the number of edge by mode
    {
        BOOST_ASSERT_MSG(nb_edge_by_mode != 0, "there should be edges in the graph");
    }

    template <typename vertex_t>
    bool operator()(const vertex_t& e) const {
        int graph_number = e / nb_edge_by_mode;

        return acceptable_modes[graph_number];
    }
};

struct GeoRefPathFinder {
    const ng::GeoRef & geo_ref;

    bool computation_launch = false;

    /// starting point
    type::GeographicalCoord start_coord;
    ng::ProjectionData starting_edge;

    /// Transportation mode
    nt::Mode_e mode;
    float speed_factor = 0.;

    /// Distance array for the Dijkstra
    std::vector<bt::time_duration> distances;

    /// Predecessors array for the Dijkstra
    std::vector<ng::vertex_t> predecessors;

    GeoRefPathFinder(const ng::GeoRef& geo_ref);

    /**
     *  Update the structure for a given starting point and transportation mode
     *  The init HAS to be called before any other methods
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode, const float speed_factor);

    /// compute the reachable stop points within the radius
    std::vector<std::pair<type::idx_t, bt::time_duration>> find_nearest_stop_points(bt::time_duration radius,
                                                                         const proximitylist::ProximityList<type::idx_t>& pl);

    /// Compute the path from the starting point to the the target geographical coord
    ng::Path compute_path(const type::GeographicalCoord& target_coord);

    /// compute the distance from the starting point to the target stop point
    bt::time_duration get_distance(type::idx_t target_idx);

    /// return the path from the starting point to the target. the target has to have been previously visited.
    ng::Path get_path(type::idx_t idx);

    /// Add the starting point projection the the path. Add a new way if needed
    void add_projections_to_path(ng::Path& p, bool append_to_begin) const;

    /**
     * Launch a dijkstra without initializing the data structure
     * Warning, it modifies the distances and the predecessors
     **/
    template<class Visitor>
    void dijkstra(ng::vertex_t start, Visitor visitor) {
        // Note: the predecessors have been updated in init
        boost::two_bit_color_map<> color(boost::num_vertices(geo_ref.graph));

        //we filter the graph to only use certain mean of transport
        using filtered_graph = boost::filtered_graph<georef::Graph, boost::keep_all, TransportationModeFilter>;
        boost::dijkstra_shortest_paths_no_init(filtered_graph(geo_ref.graph, {}, TransportationModeFilter(mode, geo_ref)),
                                               start, &predecessors[0], &distances[0],
                                               boost::get(&ng::Edge::duration, geo_ref.graph), // weigth map
                                               boost::identity_property_map(),
                                               std::less<bt::time_duration>(),
                                               SpeedDistanceCombiner(speed_factor), //we multiply the edge duration by a speed factor
                                               bt::seconds(0),
                                               visitor,
                                               color
                                               );
    }
private:
    ng::Path get_path(const ng::ProjectionData& target, std::pair<bt::time_duration, ng::vertex_t> nearest_edge);

    /** compute the path to the target and update the distances/pred
     *  return a pair with the edge corresponding to the target and the distance
     */
    std::pair<bt::time_duration, ng::vertex_t> update_path(const ng::ProjectionData& target);

    /// find the nearest vertex from the projection. return the distance to this vertex and the vertex
    std::pair<bt::time_duration, ng::vertex_t> find_nearest_vertex(const ng::ProjectionData& target) const;

};

/** Structure managing the computation on the streetnetwork */
class StreetNetwork {
public:
    StreetNetwork(const ng::GeoRef& geo_ref);

    void init(const type::EntryPoint& start_coord, boost::optional<const type::EntryPoint&> end_coord = {});

    bool departure_launched() const;
    bool arrival_launched() const;
    std::vector<std::pair<type::idx_t, bt::time_duration>> find_nearest_stop_points(
                                                    bt::time_duration radius,
                                                    const proximitylist::ProximityList<type::idx_t>& pl,
                                                    bool use_second);

    bt::time_duration get_distance(type::idx_t target_idx, bool use_second = false);

    ng::Path get_path(type::idx_t idx, bool use_second = false);

    /**
     * Build the direct path between the start and the end by connecting the 2 sub path (from departure and from arrival).
     * If the 2 sub path does not connect return an empty path
     **/
    ng::Path get_direct_path();

private:
    const ng::GeoRef & geo_ref;
    GeoRefPathFinder departure_path_finder;
    GeoRefPathFinder arrival_path_finder;
};

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};
struct DestinationNotFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<ng::vertex_t> & destinations;
    target_visitor(const std::vector<ng::vertex_t> & destinations) : destinations(destinations){}
    template <typename graph_type>
    void finish_vertex(ng::vertex_t u, const graph_type&){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    boost::posix_time::time_duration max_duration;
    const std::vector<bt::time_duration>& durations;
    distance_visitor(bt::time_duration max_dur, const std::vector<bt::time_duration> & dur) :
        max_duration(max_dur), durations(dur){}

    template <typename graph_type>
    void finish_vertex(ng::vertex_t u, const graph_type&){
        if(durations[u] > max_duration)
            throw DestinationFound();
    }
};

// Visitor who throw a DestinationFound exception when all target has been visited
struct target_all_visitor : public boost::dijkstra_visitor<> {
    std::vector<ng::vertex_t> destinations;
    size_t nbFound = 0;
    target_all_visitor(std::vector<ng::vertex_t> destinations) : destinations(destinations){}
    template <typename graph_type>
    void finish_vertex(ng::vertex_t u, const graph_type&){
        if (std::find(destinations.begin(), destinations.end(), u) != destinations.end()) {
            nbFound++;
            if (nbFound == destinations.size()) {
                throw DestinationFound();
            }
        }
    }
};

// Visiteur qui lève une exception dès que la cible souhaitée est atteinte
struct target_unique_visitor : public boost::dijkstra_visitor<> {
    const ng::vertex_t & destination;

    target_unique_visitor(const ng::vertex_t & destination) :
        destination(destination){}

    template <typename graph_type>
    void finish_vertex(ng::vertex_t u, const graph_type&){
        if(u == destination)
            throw DestinationFound();
    }
};


}}//namespace navitia::georef
