#include "georef.h"

namespace navitia { namespace georef {
/** Permet de construire un graphe de manière simple

    C'est essentiellement destiné aux tests unitaires
  */
struct GraphBuilder{
    /// Graphe que l'on veut construire
    //StreetNetwork & street_network;
    GeoRef & geo_ref;

    /// Associe une chaine de caractères à un nœud
    std::map<std::string, vertex_t> vertex_map;

    /// Le constructeur : on précise sur quel graphe on va construire
    //GraphBuilder(StreetNetwork & street_network) : street_network(street_network){}
    GraphBuilder(GeoRef & geo_ref) : geo_ref(geo_ref){}

    /// Ajoute un nœud, s'il existe déjà, les informations sont mises à jour
    GraphBuilder & add_vertex(std::string node_name, float x, float y);

    /// Ajoute un arc. Si un nœud n'existe pas, il est créé automatiquement
    /// Si la longueur n'est pas précisée, il s'agit de la longueur à vol d'oiseau
    GraphBuilder & add_edge(std::string source_name, std::string target_name, float length = -1, bool bidirectionnal = false);

    /// Surchage de la création de nœud pour plus de confort
    GraphBuilder & operator()(std::string node_name, float x, float y){ return add_vertex(node_name, x, y);}


    /// Surchage de la création d'arc pour plus de confort
    GraphBuilder & operator()(std::string source_name, std::string target_name, float length = -1, bool bidirectionnal = false){
        return add_edge(source_name, target_name, length, bidirectionnal);
    }

    /// Retourne le nœud demandé, jette une exception si on ne trouve pas
    vertex_t get(const std::string & node_name);

    /// Retourne l'arc demandé, jette une exception si on ne trouve pas
    edge_t get(const std::string & source_name, const std::string & target_name);
};

}}
