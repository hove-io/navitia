#pragma once

/** Quelques fonctions pour faciliter des traitements dans divers modules */
#include <boost/unordered_map.hpp>

namespace webservice{

    /// Tableau associatif de deux string (implémenté sous forme de hashmap)
    typedef boost::unordered_map<std::string, std::string> map;

    /** Parse les paramètres d'une query_string
      *
      * Étant une chaine "clef1=val1&clef2&clef3=val3", retourne un tableau associatif
      * "clef1" => "val1"
      * "clef2" => ""
      * "clef3" => "val3"
      */
    map parse_params(const std::string & query_string);
}
