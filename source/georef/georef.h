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

#pragma once
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"
#include "adminref.h"
#include "utils/exception.h"
#include "utils/flat_enum_map.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include "utils/serialization_vector.h"
#include <boost/serialization/utility.hpp>
#include <boost/serialization/set.hpp>
#include <map>
#include <set>
#include <functional>

namespace nt = navitia::type;
namespace nf = navitia::autocomplete;

namespace navitia {
namespace georef {

/// default speed (in m/s) by transportation mode, defined at compile time
const flat_enum_map<nt::Mode_e, float> default_speed{{{
    1.12f,   // nt::Mode_e::Walking
    4.1f,    // nt::Mode_e::Bike
    11.11f,  // nt::Mode_e::Car
    4.1f,    // nt::Mode_e::Vls
    11.11f   // nt::Mode_e::CarNoPark (must be the same as car)
}}};

/** Node properties (road intersection) */
struct Vertex {
    nt::GeographicalCoord coord;

    Vertex() {}
    Vertex(double x, double y, bool is_meters = false) : coord(x, y) {
        if (is_meters) {
            coord.set_xy(x, y);
        }
    }

    Vertex(const nt::GeographicalCoord& other) : coord(other.lon(), other.lat()) {}

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& coord;
    }
};

/** Edge properties (used to be "segment")*/

struct Edge {
    nt::idx_t way_idx = nt::invalid_idx;   //< indexing street name
    nt::idx_t geom_idx = nt::invalid_idx;  // geometry index
    navitia::time_duration duration = {};  // duration of the edge

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& way_idx& geom_idx& duration;
    }
    Edge(nt::idx_t wid, navitia::time_duration dur) : way_idx(wid), duration(dur) {}
    Edge() {}
};

// Plein de typedefs pour nous simpfilier un peu la vie

/** Définit le type de graph que l'on va utiliser
 *
 * Les arcs sortants et la liste des nœuds sont représentés par des vecteurs
 * les arcs sont orientés
 * les propriétés des nœuds et arcs sont les classes définies précédemment
 */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Vertex, Edge> Graph;

/// Représentation d'un nœud dans le g,raphe
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;

/// Représentation d'un arc dans le graphe
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

/// Pour parcourir les segements du graphe
typedef boost::graph_traits<Graph>::edge_iterator edge_iterator;

/** le numéro de la maison :
    il représente un point dans la rue, voie */
struct HouseNumber {
    nt::GeographicalCoord coord;
    int number;

    HouseNumber() : number(-1) {}
    HouseNumber(double lon, double lat, int nb) : coord(lon, lat), number(nb) {}

    bool operator<(const HouseNumber& other) const { return this->number < other.number; }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& coord& number;
    }
};

/** Nommage d'une voie (anciennement "adresse").
    Typiquement le nom de rue **/
struct Way : public nt::Nameable, nt::Header {
public:
    std::string way_type;
    std::string comment;
    // liste des admins
    std::vector<Admin*> admin_list;

    std::vector<HouseNumber> house_number_left;
    std::vector<HouseNumber> house_number_right;
    std::vector<std::pair<vertex_t, vertex_t> > edges;
    std::vector<nt::LineString> geoms;

    void add_house_number(const HouseNumber&);
    nt::GeographicalCoord nearest_coord(const int, const Graph&) const;
    // returns {house_number, distance}, return {-1, x} if not found
    std::pair<int, double> nearest_number(const nt::GeographicalCoord&) const;
    nt::GeographicalCoord projected_centroid(const Graph&) const;
    nt::MultiLineString make_multiline(const Graph&) const;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& idx& name& comment& uri& way_type& admin_list& house_number_left& house_number_right& edges& geoms;
    }
    std::string get_label() const;

    void sort_house_numbers();

#ifdef _DEBUG_DIJKSTRA_QUANTUM_
    template <typename Stream, typename G>
    void print(Stream& stream, const G& g) {
        for (auto& edge : this->edges) {
            stream << std::setprecision(16) << "LINESTRING(" << g[edge.first].coord.lon() << " "
                   << g[edge.first].coord.lat() << ", " << g[edge.second].coord.lon() << " "
                   << g[edge.second].coord.lat() << ")" << std::endl;
        }
    }
#endif

private:
    nt::GeographicalCoord get_geographical_coord(const int, const Graph&) const;
    nt::GeographicalCoord extrapol_geographical_coord(int, const Graph&) const;
};

/** Un bout d'itinéraire :
        un nom de voie et une liste de segments */
struct PathItem {
    nt::idx_t way_idx = nt::invalid_idx;            //< Way of this path item
    navitia::time_duration duration = {};           //< Length of the journey on this item
    std::deque<nt::GeographicalCoord> coordinates;  //< path item coordinates
    int angle = 0;                                  //< Angle with the next PathItem (needed to give direction)

    enum class TransportCaracteristic {
        Walk,
        Bike,
        Car,
        BssTake,         // when a bike is taken
        BssPutBack,      // when a bike is put back
        CarPark,         // when a car is parked
        CarLeaveParking  // when leaving a parking
    };
    TransportCaracteristic transportation = TransportCaracteristic::Walk;

    float get_length(float speed_factor) const;
};

/** Complete path */
struct Path {
    navitia::time_duration duration = {};  //< Total length of the path
    std::deque<PathItem> path_items = {};  //< List of street used
};

struct ProjectionData;

struct POI;
struct POIType;

/** All you need about the street network */
struct GeoRef {
    // parameters
    navitia::time_duration default_time_bss_pickup = seconds(120);
    navitia::time_duration default_time_bss_putback = seconds(60);
    navitia::time_duration default_time_parking_leave = seconds(5 * 60);
    navitia::time_duration default_time_parking_park = seconds(5 * 60);

    std::vector<POIType*> poitypes;
    std::map<std::string, POIType*> poitype_map;
    std::vector<POI*> pois;
    std::map<std::string, POI*> poi_map;
    proximitylist::ProximityList<type::idx_t> poi_proximity_list;
    std::vector<Way*> ways;
    std::map<std::string, nt::idx_t> way_map;
    std::map<std::string, nt::idx_t> admin_map;
    std::vector<Admin*> admins;

    /// Indexe sur les noms de voirie
    autocomplete::Autocomplete<unsigned int> fl_admin =
        autocomplete::Autocomplete<unsigned int>(navitia::type::Type_e::Admin);
    /// Indexe sur les noms de voirie
    autocomplete::Autocomplete<unsigned int> fl_way =
        autocomplete::Autocomplete<unsigned int>(navitia::type::Type_e::Way);

    /// Indexe sur les pois
    autocomplete::Autocomplete<unsigned int> fl_poi =
        autocomplete::Autocomplete<unsigned int>(navitia::type::Type_e::POI);

    /// Indexe tous les nœuds
    proximitylist::ProximityList<vertex_t> pl;

    /// for all stop_point, we store it's projection on each graph
    typedef flat_enum_map<nt::Mode_e, ProjectionData> ProjectionByMode;
    std::vector<ProjectionByMode> projected_stop_points = {};

    /// Graphe pour effectuer le calcul d'itinéraire
    Graph graph;

    /*
     * We have 3 graphs :
     *  1/ for walking
     *  2/ for biking
     *  3/ for driving
     *
     *  But 4 transportation mode cf explanation in init()
     */
    flat_enum_map<nt::Mode_e, nt::idx_t> offsets;

    /// number of vertex by transportation mode
    nt::idx_t nb_vertex_by_mode = 0;
    navitia::autocomplete::autocomplete_map synonyms;
    std::set<std::string> ghostwords;

    int word_weight = 5;  // Pas serialisé : lu dans le fichier ini

    void init();

    template <class Archive>
    void save(Archive& ar, const unsigned int) const {
        ar& ways& way_map& graph& offsets& fl_admin& fl_way& pl& projected_stop_points& admins& admin_map& pois& fl_poi&
            poitypes& poitype_map& poi_map& synonyms& ghostwords& poi_proximity_list& nb_vertex_by_mode;
    }

    template <class Archive>
    void load(Archive& ar, const unsigned int) {
        // La désérialisation d'une boost adjacency list ne vide pas le graphe
        // On avait donc une fuite de mémoire
        graph.clear();
        ar& ways& way_map& graph& offsets& fl_admin& fl_way& pl& projected_stop_points& admins& admin_map& pois& fl_poi&
            poitypes& poitype_map& poi_map& synonyms& ghostwords& poi_proximity_list& nb_vertex_by_mode;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    /** Construit l'indexe spatial */
    void build_proximity_list();

    ///  Construit l'indexe autocomplete à partir des rues
    void build_autocomplete_list();

    /// Normalisation des codes externes
    void normalize_extcode_way();
    /// Normalisation des codes externes des admins
    void build_admin_map();

    /// Chargement de la liste map code externe idx sur poitype et poi
    void build_poitypes_map();
    void build_pois_map();

    /// Recherche d'une adresse avec un numéro en utilisant Autocomplete
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> find_ways(const std::string& str,
                                                                   const int nbmax,
                                                                   const int search_type,
                                                                   std::function<bool(nt::idx_t)> keep_element,
                                                                   const std::set<std::string>& ghostwords) const;
    std::vector<Admin*> find_admins(const type::GeographicalCoord&) const;
    std::vector<Admin*> find_admins(const type::GeographicalCoord&, AdminRtree&) const;

    /**
     * Project each stop_point on the georef network
     */
    void project_stop_points(const std::vector<type::StopPoint*>& stop_points);

    /** project the stop point on all transportation mode
     * return a pair with :
     * - the projected array
     * - a boolean corresponding to the fact that at least one projection has been found
     */
    std::pair<ProjectionByMode, bool> project_stop_point(const type::StopPoint* stop_point) const;

    /** Retourne l'arc (segment) le plus proche
     *
     * Pour le trouver, on cherche le nœud le plus proche, puis pour chaque arc adjacent, on garde le plus proche
     * Ce n'est donc pas optimal, mais pour améliorer ça, il faudrait indexer des segments, ou ratisser plus large
     */

    vertex_t nearest_vertex(const type::GeographicalCoord& coordinates,
                            const proximitylist::ProximityList<vertex_t>& prox) const;
    edge_t nearest_edge(const type::GeographicalCoord& coordinates) const;
    edge_t nearest_edge(const type::GeographicalCoord& coordinates,
                        const proximitylist::ProximityList<vertex_t>& prox,
                        type::idx_t offset = 0,
                        double horizon = 500) const;

    edge_t nearest_edge(const type::GeographicalCoord& coordinates, type::Mode_e mode) const {
        return nearest_edge(coordinates, pl, offsets[mode]);
    }
    std::pair<int, const Way*> nearest_addr(const type::GeographicalCoord&) const;
    std::pair<int, const Way*> nearest_addr(const type::GeographicalCoord& coord,
                                            const std::function<bool(const Way&)>& filter) const;

    // Return false if we didn't find any projection
    bool add_bss_edges(const type::GeographicalCoord&);

    // Return false if we didn't find any projection
    bool add_parking_edges(const type::GeographicalCoord&);

    /// get the transportation mode of the vertex
    type::Mode_e get_mode(vertex_t vertex) const;
    PathItem::TransportCaracteristic get_caracteristic(edge_t edge) const;
    ~GeoRef();
    GeoRef() = default;
    GeoRef(const GeoRef& other) = default;
};

/** When given a coordinate, we have to associate it with the street network.
 *
 * This structure handle this.
 *
 * It contains
 *   - 2 possible nodes (each end of the edge where the coordinate has been projected)
 *   - the coordinate of the projection
 *   - the 2 distances between the projected point and the ends (NOTE, this is not the distance between the coordinate
 * and the ends)
 *
 */
struct ProjectionData {
    /// enum used to acces the nodes and the distances
    enum class Direction { Source = 0, Target, size };
    /// 2 possible nodes (each end of the edge where the coordinate has been projected)
    flat_enum_map<Direction, vertex_t> vertices;

    // The edge we projected on. Needed since we can't be sure to get the right edge with only the source and the target
    // because of parallel edges.
    Edge edge;

    /// has the projection been successful?
    bool found = false;

    /// The coordinate projected on the edge
    type::GeographicalCoord projected;

    // the original coordinate before projection
    type::GeographicalCoord real_coord;

    /// Distance between the projected point and the ends
    flat_enum_map<Direction, double> distances{{{-1, -1}}};

    ProjectionData() {}
    /// Project the coordinate on the graph
    ProjectionData(const type::GeographicalCoord& coord,
                   const GeoRef& sn,
                   const proximitylist::ProximityList<vertex_t>& prox);
    /// Project the coordinate on the graph corresponding to the transportation mode of the offset
    ProjectionData(const type::GeographicalCoord& coord,
                   const GeoRef& sn,
                   type::idx_t offset,
                   const proximitylist::ProximityList<vertex_t>& prox,
                   double horizon = 500);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& vertices& projected& distances& found& real_coord& edge;
    }

    void init(const type::GeographicalCoord& coord, const GeoRef& sn, edge_t nearest_edge);

    /// syntaxic sugar
    vertex_t operator[](Direction d) const { return vertices[d]; }
};

/** Nommage d'un POI (point of interest). **/
struct POIType : public nt::Nameable, nt::Header {
    const static type::Type_e type = type::Type_e::POIType;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& idx& uri& name;
    }

    type::Indexes get(type::Type_e type, const GeoRef& data) const;
};

/** Nommage d'un POI (point of interest). **/
struct POI : public nt::Nameable, nt::Header {
    const static type::Type_e type = type::Type_e::POI;
    int weight;
    nt::GeographicalCoord coord;
    std::vector<Admin*> admin_list;
    std::map<std::string, std::string> properties;
    nt::idx_t poitype_idx;
    int address_number;
    std::string address_name;
    std::string label;

    POI() : weight(0), poitype_idx(type::invalid_idx), address_number(-1) {}

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& idx& uri& name& weight& coord& admin_list& properties& poitype_idx& visible& address_number& address_name&
            label;
    }

    type::Indexes get(type::Type_e type, const GeoRef&) const;

private:
};

int compute_directions(const navitia::georef::Path& path, const nt::GeographicalCoord& c_coord);

}  // namespace georef
}  // namespace navitia
