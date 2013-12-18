#pragma once
#include "georef.h"

namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    double max_distance;
    const std::vector<float>& distances;
    distance_visitor(float max_distance, const std::vector<float> & distances) :
        max_distance(max_distance), distances(distances){}

    void finish_vertex(ng::vertex_t u, const ng::Graph&){
        if(distances[u] > max_distance)
            throw ng::DestinationFound();
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

    /// Distance array for the Dijkstra
    std::vector<float> distances;

    /// Predecessors array for the Dijkstra
    std::vector<ng::vertex_t> predecessors;

    GeoRefPathFinder(const ng::GeoRef& geo_ref);

    /**
     *  Update the structure for a given starting point and transportation mode
     *  The init HAS to be called before any other methods
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode);

    /// compute the reachable stop points within the radius
    std::vector<std::pair<type::idx_t, double>> find_nearest_stop_points(double radius,
                                                                         const proximitylist::ProximityList<type::idx_t>& pl);

    /// Compute the path from the starting point to the the target geographical coord
    ng::Path compute_path(const type::GeographicalCoord& target_coord);

    /// compute the distance from the starting point to the target stop point
    double get_distance(type::idx_t target_idx);

    /// return the path from the starting point to the target. the target has to have been previously visited.
    ng::Path get_path(type::idx_t idx);

    /// Add the starting point projection the the path. Add a new way if needed
    void add_projections_to_path(ng::Path& p) const;

private:
    ng::Path get_path(const ng::ProjectionData& target, std::pair<double, ng::vertex_t> nearest_edge);

    /** compute the path to the target and update the distances/pred
     *  return a pair with the edge corresponding to the target and the distance
     */
    std::pair<double, ng::vertex_t> update_path(const ng::ProjectionData& target);

    /// find the nearest vertex from the projection. return the distance to this vertex and the vertex
    std::pair<double, ng::vertex_t> find_nearest_vertex(const ng::ProjectionData& target) const;

};

/** Structure managing the computation on the streetnetwork */
class StreetNetwork {
public:
    StreetNetwork(const ng::GeoRef& geo_ref);

    void init(const type::EntryPoint& start_coord, boost::optional<const type::EntryPoint&> end_coord = {});

    bool departure_launched() const;
    bool arrival_launched() const;
    std::vector<std::pair<type::idx_t, double>> find_nearest_stop_points(
                                                    double radius,
                                                    const proximitylist::ProximityList<type::idx_t>& pl,
                                                    bool use_second);

    double get_distance(type::idx_t target_idx, bool use_second = false);

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


}}//namespace navitia::georef
