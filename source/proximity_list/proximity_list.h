#pragma once

#include "type.h"
using navitia::type::GeographicalCoord;
#include <vector>

/** Définit un indexe spatial qui permet de retrouver les n éléments les plus proches
 *
 * Le template T est le type que l'on souhaite indexer (typiquement un Idx). L'élément sera copié.
 *
 * On rajoute des élements itérativements et on appelle build pour construire l'indexe
 *
 * L'implémentation est un k-d tree http://en.wikipedia.org/wiki/K-d_tree.
 *
 * L'arbre stocké dans un seul vector pour des raisons de mémoire et performance
 */
template<class T>
struct ProximityList
{
    /// Élement que l'on garde dans le vector 
    struct Item
    {
        GeographicalCoord coord;
        T element;
        Item(GeographicalCoord, T element) : coord(coord), element(element) {}
    };

    /// Contient toutes les coordonnées de manière à trouver rapidement
    std::vector<Item> items;

    /// Rajoute un nouvel élément. Attention, il faut appeler build avant de pouvoir utiliser la structure
    void add(GeographicalCoord, T element){
        items.push_back(Item(coord,element));
    };

    /// Construit l'indexe
    void build(){
    }

    /// Retourne tous les éléments dans un rayon de x mètres
    std::vector<T> find_within(GeographicalCoord coord, double meters){
        return std::vector<T>();
    }

    /// Retourne les k-éléments les plus proches
    std::vector<T> find_k_nearest(GeographicalCoord coord){
        return std::vector<T>();
    }


};
