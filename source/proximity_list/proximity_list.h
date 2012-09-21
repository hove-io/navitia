#pragma once

#include "type/type.h"
#include <vector>
#include <cmath>

namespace navitia { namespace proximitylist {

using type::GeographicalCoord;
struct NotFound : public std::exception{};

/** Définit un indexe spatial qui permet de retrouver les n éléments les plus proches
 *
 * Le template T est le type que l'on souhaite indexer (typiquement un Idx). L'élément sera copié.
 * On rajoute des élements itérativements et on appelle build pour construire l'indexe.
 * L'implémentation est un bête tableau trié par X.
 * On cherche les bornes inf/sup selon X, puis on itère sur les données et on garde les bonnes
 */

template<class T>
struct ProximityList
{
    /// Élement que l'on garde dans le vector 
    struct Item {
        GeographicalCoord coord;
        T element;
        Item(){}
        Item(GeographicalCoord coord, T element) : coord(coord), element(element) {}
        template<class Archive> void serialize(Archive & ar, const unsigned int) {
            ar & coord & element;
        }
    };

    /// Contient toutes les coordonnées de manière à trouver rapidement
    std::vector<Item> items;

    /// Rajoute un nouvel élément. Attention, il faut appeler build avant de pouvoir utiliser la structure
    void add(GeographicalCoord coord, T element){
        items.push_back(Item(coord,element));
    }

    /// Construit l'indexe
    void build(){
        std::sort(items.begin(), items.end(), [](const Item & a, const Item & b){return a.coord.x < b.coord.x;});
    }

    /// Retourne tous les éléments dans un rayon de x mètres
    std::vector< std::pair<T, GeographicalCoord> > find_within(GeographicalCoord coord, double distance ) const {
        double distance_degree = distance;
        double coslat = 1;
        if(coord.degrees){
            double DEG_TO_RAD = 0.0174532925199432958;
            coslat = ::cos(coord.y * DEG_TO_RAD);
            distance_degree = distance / 111320; // Nombre de mètres dans un ° à l'Équateur
        }
        auto begin = std::lower_bound(items.begin(), items.end(), coord, [distance_degree,coslat](const Item & i, const GeographicalCoord & coord){return i.coord.x < coord.x - coslat * distance_degree;});
        auto end = std::upper_bound(items.begin(), items.end(), coord, [distance_degree,coslat](const GeographicalCoord &coord, const Item & i){return coord.x + distance_degree * coslat < i.coord.x ;});
        std::vector< std::pair<T, GeographicalCoord> > result;
        double max_dist = distance * distance;
        for(; begin != end; ++begin){
            if(begin->coord.approx_sqr_distance(coord, coslat) <= max_dist)
                result.push_back(std::make_pair(begin->element, begin->coord));
        }
        std::sort(result.begin(), result.end(), [&coord, &coslat](const std::pair<T, GeographicalCoord> & a, const std::pair<T, GeographicalCoord> & b){return a.second.approx_sqr_distance(coord, coslat) < b.second.approx_sqr_distance(coord, coslat);});
        return result;
    }

    /// Retourne les k-éléments les plus proches
    std::vector<T> find_k_nearest(GeographicalCoord , size_t ) const {
        return std::vector<T>();
    }

    /// Fonction de confort pour retrouver l'élément le plus proche dans l'indexe
    T find_nearest(double lon, double lat) const {
        return find_nearest(GeographicalCoord(lon, lat));
    }

    /// Retourne l'élément le plus proche dans tout l'indexe
    T find_nearest(GeographicalCoord coord) const {
        if(items.size() == 0) throw NotFound();
        auto nearest_x = std::lower_bound(items.begin(), items.end(), coord, [](const Item & i, const GeographicalCoord & coord){return i.coord.x < coord.x;});

        Item nearest =  nearest_x == items.end() ? items.back() : *nearest_x;
        T best_item = nearest.element;

        double coslat = coord.degrees ? ::cos(coord.y * 0.0174532925199432958) : 1;
        double best_sqr_dist = coord.approx_sqr_distance(nearest.coord, coslat);
        double dist_degrees = ::sqrt(best_sqr_dist);
        if(coord.degrees){
            dist_degrees = coslat * dist_degrees / 111320; // Nombre de mètres dans un ° à l'Équateur
        }
        auto begin = std::lower_bound(items.begin(), items.end(), coord, [dist_degrees](const Item & i, const GeographicalCoord & coord){return i.coord.x < coord.x - dist_degrees;});
        auto end = std::upper_bound(items.begin(), items.end(), coord, [dist_degrees](const GeographicalCoord &coord, const Item & i){return coord.x + dist_degrees  < i.coord.x ;});

        for(; begin != end; ++begin){
            double current_dist = begin->coord.approx_sqr_distance(coord, coslat);
            if(current_dist < best_sqr_dist) {
                best_item = begin->element;
                best_sqr_dist = current_dist;
            }
        }
        return best_item;
    }


    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & items;
    }

};

}} // namespace navitia::proximitylist
