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

    /// Points de départ et d'arrivée fournis par la requête
    type::GeographicalCoord start_coord;
    ng::ProjectionData starting_edge;

    nt::Mode_e mode;

    /// Tableau des distances utilisé par Dijkstra
    std::vector<float> distances;

    /// Tableau des prédécesseurs utilisé par Dijkstra
    std::vector<ng::vertex_t> predecessors;

    GeoRefPathFinder(const ng::GeoRef& geo_ref);
    /**
     *  Met à jour les indicateurs pour savoir si les calculs ont été lancés
     */
    void init(const type::GeographicalCoord& start_coord, nt::Mode_e mode);

    /**
     *Calcule quels sont les stop point atteignables en radius mètres de marche à pied
     */
    std::vector<std::pair<type::idx_t, double>> find_nearest_stop_points(double radius,
                                                                         const proximitylist::ProximityList<type::idx_t>& pl);

    double get_distance(type::idx_t target_idx);

    ng::Path get_path(type::idx_t idx);
};

/** Structures avec toutes les données écriture pour streetnetwork */
class StreetNetwork {
public:
    StreetNetwork(const ng::GeoRef& geo_ref);

    /**
     *  Met à jour les indicateurs pour savoir si les calculs ont été lancés
     */
    void init(const type::EntryPoint& start_coord, boost::optional<const type::EntryPoint&> end_coord = {});

    /**
     * Indique si le calcul itinéraire piéton de départ a été lancé
     */
    bool departure_launched() const;
    /**
     * Indique si le calcul itinéraire piéton de fin a été lancé
     */
    bool arrival_launched() const;

    /** Calcule quels sont les stop point atteignables en radius mètres de marche à pied
     */
    std::vector<std::pair<type::idx_t, double>> find_nearest_stop_points(
                                                    double radius,
                                                    const proximitylist::ProximityList<type::idx_t>& pl,
                                                    bool use_second);

    double get_distance(type::idx_t target_idx, bool use_second = false);

    /**
     * Reconstruit l'itinéraire piéton à partir de l'idx
     */
    ng::Path get_path(type::idx_t idx, bool use_second = false);

    /**
     *Construit l'itinéraire piéton direct. Le path est vide s'il n'existe pas
     **/
    ng::Path get_direct_path();

private:
    const ng::GeoRef & geo_ref;
    GeoRefPathFinder departure_path_finder;
    GeoRefPathFinder arrival_path_finder;
};


}}//namespace navitia::georef
