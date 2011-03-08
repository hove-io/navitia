#pragma once

#include <map>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/thread/shared_mutex.hpp>

/** Configuration est un singleton : c'est à dire qu'on l'utilise 
  * Configuration * conf = Configuration::get();
  * Au premier appel une instance sera créée
  * Aux appels suivants, la même instance sera utilisée
  * Cela permet d'avoir une configuration unique sans utiliser de variables globales
  */
struct Configuration {
private:
    static Configuration * instance;
    /// Dictionnaire de chaînes de caractères
    /// Par défaut contient au moins :
    /// application : le nom de la dll ou de l'exécutable
    /// path : le chemin où se situe la dll ou l'exécutable
    std::map<std::string, std::string> strings;
    std::map<std::string, int> ints;
    std::map<std::string, float> floats;

    /// Mutex permetant multiple-read/single-write
    boost::shared_mutex mutex;

public:
    /// Premet de récupérer l'instance de configuration
    static Configuration * get();

    /// Récupère une chaîne de caractères
    std::string get_string(const std::string & key);

    /// Récupère un entier
    int get_int(const std::string & key);

    void set_string(const std::string & key, const std::string & value);
    void set_int(const std::string & key, int value);

    /// Contient les données du fichier ini
    /// Pour y accéder ini[section][clef]
    /// Si la clef ou la section n'existe pas, retourne une string vide
    /// C'est à l'utilisateur de faire la conversion qui l'intéresse
    std::map< std::string, std::map<std::string, std::string> > ini;

    /// Retourne vrai si la section existe
    bool has_section(const std::string & section_name);

    /// Exception levée lorsque qu'on ne trouve pas un élément
    struct not_found{};

    /// Cherche une clef dans une section et effectue la conversion
    /// Lève une exception si :
    /// — la clef ou la section n'existe pas
    /// — la conversion est impossible
    template<class T>
    T get_as(const std::string & section, const std::string & key) {
        auto section_it = ini.find(section);
        if(section_it == ini.end())
            throw not_found();
        auto element_it = section_it->second.find(key);
        if(element_it == section_it->second.end())
            throw not_found();
        return boost::lexical_cast<T>(element_it->second);
    }

    /// Charge un fichier ini
    /// Il est possible de charger plusieurs fichiers
    /// Si dans deux fichiers il y a la même section et la même clef, la valeur sera écrasée
    void load_ini(const std::string & filename);
};

