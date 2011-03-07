#pragma once

#include <map>
#include <string>

/** Configuration est un singleton : c'est à dire qu'on l'utilise 
  * Configuration * conf = Configuration::get();
  * Au premier appel une instance sera créée
  * Aux appels suivants, la même instance sera utilisée
  * Cela permet d'avoir une configuration unique sans utiliser de variables globales
  */
struct Configuration {
        static Configuration * instance;
    public:
        std::map<std::string, std::string> strings;
        std::map<std::string, int> ints;
        std::map<std::string, float> floats;

        static Configuration * get();
};

