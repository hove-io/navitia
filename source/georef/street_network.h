#pragma once
#include "georef.h"

namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {

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

    /**
     * Launch a dijkstra without initializing the data structure
     * Warning, it modifies the distances and the predecessors
     **/
    template<class Visitor>
    void dijkstra(ng::vertex_t start, Visitor visitor) {
        predecessors[start] = start;
        boost::two_bit_color_map<> color(boost::num_vertices(geo_ref.graph));
        boost::dijkstra_shortest_paths_no_init(geo_ref.graph, start, &predecessors[0], &distances[0],
                                               boost::get(&ng::Edge::length, geo_ref.graph), // weigth map
                                               boost::identity_property_map(),
                                               std::less<float>(), boost::closed_plus<float>(),
                                               0,
                                               visitor,
                                               color
                                               );
    }
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

// Exception levée dès que l'on trouve une destination
struct DestinationFound{};
struct DestinationNotFound{};

// Visiteur qui lève une exception dès qu'une des cibles souhaitées est atteinte
struct target_visitor : public boost::dijkstra_visitor<> {
    const std::vector<ng::vertex_t> & destinations;
    target_visitor(const std::vector<ng::vertex_t> & destinations) : destinations(destinations){}
    void finish_vertex(ng::vertex_t u, const ng::Graph&){
        if(std::find(destinations.begin(), destinations.end(), u) != destinations.end())
            throw DestinationFound();
    }
};

// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    double max_distance;
    const std::vector<float>& distances;
    distance_visitor(float max_distance, const std::vector<float> & distances) :
        max_distance(max_distance), distances(distances){}

    void finish_vertex(ng::vertex_t u, const ng::Graph&){
        if(distances[u] > max_distance)
            throw DestinationFound();
    }
};

// Visitor who throw a DestinationFound exception when all target has been visited
struct target_all_visitor : public boost::dijkstra_visitor<> {
    std::vector<ng::vertex_t> destinations;
    size_t nbFound = 0;
    target_all_visitor(std::vector<ng::vertex_t> destinations) : destinations(destinations){}
    void finish_vertex(ng::vertex_t u, const ng::Graph&){
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

    void finish_vertex(ng::vertex_t u, const ng::Graph&){
        if(u == destination)
            throw DestinationFound();
    }
};


}}//namespace navitia::georef
