#pragma once
#include <vector>
#include <boost/bimap.hpp>

namespace georef { namespace type {

typedef unsigned int idx_t;

const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

/// Propriétes du nom d'un objet
struct Nameable{
    std::string name;
    std::string comment;
};

/// Propriétes de l'entête d'un objet
struct GeoRefHeader{
    std::string id;
    idx_t idx;
    std::string external_code;

    GeoRefHeader() : idx(invalid_idx){}
};

/// Propriétes de l'objet représentant les coordonnées
struct GeographicalCoord{
    double x;
    double y;

    /// Est-ce que les coordonnées sont en degres, par défaut on suppose que oui
    /// Cela a des impacts sur le calcul des distances
    /// Si ce n'est pas des degrés, on prend la distance euclidienne
    bool degrees;

    GeographicalCoord() : x(0), y(0), degrees(true) {}
    GeographicalCoord(double x, double y, bool degrees = true) : x(x), y(y), degrees(degrees) {}
};

/// Propriétes de l'objet pays
struct Country : public Nameable, GeoRefHeader{
    std::vector<idx_t> district_list;

    Country(){}
};

/// Propriétes de l'objet district
struct District : public Nameable, GeoRefHeader{
    std::vector<idx_t> department_list;
    idx_t country_idx;

    District() : country_idx(invalid_idx){}
};

/// Propriétes de l'objet Departement
struct Department : public Nameable, GeoRefHeader{
    std::vector<idx_t> city_list; // liste des communes
    idx_t district_idx;

    Department(): district_idx(invalid_idx){}
};

/// Propriétes de l'objet commune
struct City : public Nameable, GeoRefHeader{
    std::vector<idx_t> way_list; // liste des rues
    idx_t department_idx;
    GeographicalCoord city_coord;

    City(): department_idx(invalid_idx){}
};

/// Propriétes de l'objet Nœud : point d'intersection de deux (ou plus) rues
struct Vertex : public Nameable, GeoRefHeader {
    GeographicalCoord vertex_coord;

    Vertex(){}
};

/// Propriétes de l'objet arc : segment
struct Edge: public Nameable, GeoRefHeader {    
    float length; //< longeur en mètres de l'arc
    bool cyclable; //< est-ce que le segment est accessible à vélo ?
    int start_number; //< numéro de rue au début du segment
    int end_number; //< numéro de rue en fin de segment

    Edge() : length(0), cyclable(false), start_number(-1), end_number(-1){}
};

/// Propriétes de l'objet rue : adresse
struct Way : public Nameable, GeoRefHeader{
    std::vector< idx_t > edge_list; // liste des segments
    GeographicalCoord way_coord;    
    idx_t city_idx;

    Way(): city_idx(invalid_idx){}
};

}}
