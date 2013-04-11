#pragma once
#include "type/type.h"
#include "routing/routing.h"
#include <vector>
namespace navitia { namespace routing {
    class RAPTOR;
     ///Construit un chemin, utilisé lorsque l'algorithme a été fait en sens anti-horaire
    Path makePathreverse(unsigned int destination_idx, unsigned int countb,
                         const RAPTOR &raptor_);

    ///Construit un chemin
    Path makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise,
                  const RAPTOR &raptor_);


    ///Construit tous chemins trouvés
    std::vector<Path> 
    makePathes(std::vector<std::pair<type::idx_t, double> > destinations,
               navitia::type::DateTime dt, const float walking_speed, 
               const RAPTOR &raptor_);
    ///Construit tous les chemins trouvés, lorsque le calcul est lancé dans le sens inverse
    std::vector<Path> 
    makePathesreverse(std::vector<std::pair<type::idx_t, double> > destinations,
                      navitia::type::DateTime dt, const float walking_speed,
                      const RAPTOR &raptor_);
    void patch_datetimes(Path &path);

}}
