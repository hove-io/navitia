#pragma once
#include "georef.h"



namespace ng = navitia::georef;

namespace navitia { namespace streetnetwork {


// Visiteur qui s'arrête au bout d'une certaine distance
struct distance_visitor : public boost::dijkstra_visitor<> {
    double max_distance;
    const std::vector<float> & distances;
    distance_visitor(float max_distance, const std::vector<float> & distances) : max_distance(max_distance), distances(distances){}
    void finish_vertex(ng::vertex_t u, const ng::Graph&){
        if(distances[u] > max_distance)
            throw ng::DestinationFound();
    }
};

/** Structures avec toutes les données écriture pour streetnetwork */
struct StreetNetwork {
public:

    StreetNetwork(const ng::GeoRef & geo_ref);

    /**
     *  Met à jour les indicateurs pour savoir si les calculs ont été lancés
     *
     */
    void init();

    /**
     * Indique si le calcul itinéraire piéton de départ a été lancé
     */
    bool departure_launched();
    /**
     * Indique si le calcul itinéraire piéton de fin a été lancé
     */
    bool arrival_launched();

    /** Calcule quels sont les stop point atteignables en radius mètres de marche à pied
     */
    std::vector< std::pair<type::idx_t, double> > find_nearest_stop_points(const type::GeographicalCoord & start_coord, const proximitylist::ProximityList<type::idx_t> & pl, double radius, bool use_second = false);

    /// Reconstruit l'itinéraire piéton à partir de l'idx
    ng::Path get_path(type::idx_t idx, bool use_second = false);

    /// Construit l'itinéraire piéton direct. Le path est vide s'il n'existe pas
    ng::Path get_direct_path();

private:
    const ng::GeoRef & geo_ref;

    std::vector< std::pair<type::idx_t, double> > find_nearest_stop_points(const ng::ProjectionData & start, double radius,
                                                                           const std::vector< std::pair<type::idx_t, type::GeographicalCoord> > & elements,
                                                                           std::vector<float> & dist,
                                                                           std::vector<ng::vertex_t> & preds,
                                                                           std::map<type::idx_t, ng::ProjectionData> & idx_proj);


    /// Point de départ et d'arrivée fourni par la requête
    ng::ProjectionData start;
    ng::ProjectionData destination;

    // Les données sont doublées pour garder les données au départ et à l'arrivée
    /// Tableau des distances utilisé par Dijkstra
    std::vector<float> distances;
    std::vector<float> distances2;

    /// Tableau des prédécesseurs utilisé par Dijkstra
    std::vector<ng::vertex_t> predecessors;
    std::vector<ng::vertex_t> predecessors2;

    /// Associe chaque idx_t aux données de projection sur le filaire associées
    std::map<type::idx_t, ng::ProjectionData> idx_projection;
    std::map<type::idx_t, ng::ProjectionData> idx_projection2;

    /// Savoir si les calculs ont été lancés en début et fin
    bool departure_launch;
    bool arrival_launch;
};
}}
