
#pragma once

#include "georef/edge.h"
#include <boost/graph/adjacency_list.hpp>

namespace navitia {
namespace georef {

struct Vertex;

// Plein de typedefs pour nous simplifier un peu la vie

/** Définit le type de graph que l'on va utiliser
 *
 * Les arcs sortants et la liste des nœuds sont représentés par des vecteurs
 * les arcs sont orientés
 * les propriétés des nœuds et arcs sont les classes définies précédemment
 */
using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Vertex, Edge>;

/// Représentation d'un nœud dans le g,raphe
using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;

/// Représentation d'un arc dans le graphe
using edge_t = boost::graph_traits<Graph>::edge_descriptor;

/// Pour parcourir les segements du graphe
using edge_iterator = boost::graph_traits<Graph>::edge_iterator;

}  // namespace georef
}  // namespace navitia
