#pragma once

#include "type/type.h"
using navitia::type::GeographicalCoord;
#include <vector>
#include <cmath>

struct NotFound : public std::exception{};

/** Définit un indexe spatial qui permet de retrouver les n éléments les plus proches
 *
 * Le template T est le type que l'on souhaite indexer (typiquement un Idx). L'élément sera copié.
 * On rajoute des élements itérativements et on appelle build pour construire l'indexe.
 * L'implémentation est un k-d tree http://en.wikipedia.org/wiki/K-d_tree.
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
        Item(){}
        Item(GeographicalCoord coord, T element) : coord(coord), element(element) {}
        template<class Archive> void serialize(Archive & ar, const unsigned int) {
            ar & coord & element;
        }

    };

    /// Contient toutes les coordonnées de manière à trouver rapidement
    std::vector<Item> items;

    typedef typename std::vector<Item>::iterator iterator;

    /// Rajoute un nouvel élément. Attention, il faut appeler build avant de pouvoir utiliser la structure
    void add(GeographicalCoord coord, T element){
        items.push_back(Item(coord,element));
    }

    /// Construit l'indexe
    void build(){
        build(items.begin(), items.end(), true);
    }

    /** Construit l'indexe partiellement, sur une dimension
      * On découpe récursivement l'espace en deux de manière à ce que les éléments à gauche soient plus petits
      * que l'élément médian, et que les éléments à droite soient plus grand
      */
    void build(iterator begin, iterator end, bool along_x){
        if(end - begin <= 1) return;

        // On trie selon la bonne dimension
        if(along_x)
            std::sort(begin, end, [](Item a, Item b){return a.coord.x < b.coord.x;});
        else
            std::sort(begin, end, [](Item a, Item b){return a.coord.y < b.coord.y;});

        // On récupére l'élément médian
        iterator median = begin + (end - begin) / 2;
        build(begin, median, !along_x);
        build(median + 1, end, !along_x);
    }

    /// Retourne tous les éléments dans un rayon de x mètres
    std::vector<T> find_within(GeographicalCoord , double ){
        return std::vector<T>();
    }

    /// Retourne les k-éléments les plus proches
    std::vector<T> find_k_nearest(GeographicalCoord , size_t ){
        return std::vector<T>();
    }

    /// Retourne l'élément le plus proche dans tout l'indexe
    T find_nearest(GeographicalCoord coord){
        return find_nearest(coord, items.begin(), items.end(), true).first;
    }

    /// Retourne l'élément le plus proche dans un espace restreint et sa distance à la cible
    std::pair<T, double> find_nearest(GeographicalCoord coord, iterator begin, iterator end, bool along_x){
        if(end - begin == 0) throw NotFound();
        if(end - begin == 1)
            return std::make_pair(begin->element, coord.distance_to(begin->coord));

        iterator median = begin + (end - begin) / 2;

        // On détermine de quel côté de l'élément médian on regarde
        bool left;
        if(along_x) left = coord.x < median->coord.x;
        else left = coord.y < median->coord.y;

        // On cherche récursivement dans la moitié le meilleur élément
        std::pair<T, double> best;
        if(left) best = find_nearest(coord, begin, median, !along_x);
        else best = find_nearest(coord, median, end, !along_x);

        // On regarde si de l'autre côté ça peut être plus intéressant
        // Pour que ce soit plus intéressant, il faut que la meilleur distance soit supérieure
        // à la projection du median suivant le bon axe
        GeographicalCoord projected = median->coord;
        if(along_x) projected.y = coord.y;
        else projected.y = coord.y;

        bool other_half = coord.distance_to(projected) < best.second;

        std::pair<T, double> other_best;
        if(other_half){
            if(!left) other_best = find_nearest(coord, begin, median, !along_x);
            else other_best = find_nearest(coord, median, end, !along_x);

            // On ne garde que le meilleur des deux
            if(other_best.second < best.second)
                best = other_best;
        }
        return best;
    }

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & items;
    }

};
