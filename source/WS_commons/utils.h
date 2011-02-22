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
//		  section.key	, value
/*
namespace IniFile{
	typedef std::map<std::string, boost::variant<std::string, std::string> > SettingsMap;
	SettingsMap parse_ini_file(std::string filename);
}
*/

// formatge d'un réel

