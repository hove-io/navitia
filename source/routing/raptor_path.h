#pragma once
#include "type/type.h"
#include "routing/routing.h"
#include <vector>
namespace navitia { namespace routing {
    class RAPTOR;
     ///Construit un chemin, utilisé lorsque l'algorithme a été fait en sens anti-horaire
    Path makePathreverse(unsigned int destination_idx, unsigned int countb, const type::AccessibiliteParams & accessibilite_params,
                         const RAPTOR &raptor_, bool without_disrupt);

    ///Construit un chemin
    Path makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise, bool without_disrupt, const type::AccessibiliteParams & accessibilite_params/*const type::Properties &required_properties*/,
                  const RAPTOR &raptor_);


    ///Construit tous chemins trouvés
    std::vector<Path> 
    makePathes(std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > destinations,
               DateTime dt,
//               const type::Properties &required_properties,
               const type::AccessibiliteParams & accessibilite_params,
               const RAPTOR &raptor_, bool clockwise, bool without_disrupt);

    /// Ajuste les temps d’attente
    void patch_datetimes(Path &path);
}}
