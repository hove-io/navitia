#pragma once
#include<boost/foreach.hpp>
#include<boost/tokenizer.hpp>
#include<map>
#include<set>


/** Map de type first letter
  *
  * On associe une chaine de caractères, par exemple "rue jean jaures" à une valeur T (typiquement un pointeur
  * ou un indexe)
  *
  * Quand on cherche "r jean" on veut récupérer la valeur correspondant à la clef "rue jean jaures" et "rue jeanne d'arc"
  */
template<class T>
struct FirstLetter
{    
     /// Structure temporaire pour construire l'indexe
    std::map<std::string, std::set<T> > map;

    /// À chaque mot (par exemple "rue" ou "jaures") on associe un tableau de T qui contient la liste des éléments contenant ce mot
    typedef std::pair<std::string, std::vector<T> > vec_elt;

    /// Structure principale de notre indexe
    std::vector<vec_elt> vec_map;

    // Méthodes permettant de construire l'indexe

    /** Étant donné une chaîne de caractères et la position de l'élément qui nous intéresse :
      * – on découpe en mots la chaîne (tokens)
      * — on rajoute la position à la liste de chaque mot
      */
    void add_string(const std::string & str, T position){
        boost::tokenizer<> tokens(str);
        BOOST_FOREACH(auto token, tokens){
            map[token].insert(position);
        }
    }

    /** Construit la structure finale
      *
      * Les map et les set sont bien pratiques, mais leurs performances sont mauvaises avec des petites données (comme des ints)
      */
    void build(){
        vec_map.reserve(map.size());
        BOOST_FOREACH(auto key_val, map){
            vec_map.push_back(std::make_pair(key_val.first, std::vector<T>(key_val.second.begin(), key_val.second.end())));
        }
    }


    // Méthodes premettant de retrouver nos éléments

    /** Définit un fonctor permettant de parcourir notre structure un peu particulière */
    struct comp{
        /** Utilisé pour trouver la borne inf. Quand on cherche av, on veux que avenue soit également trouvé
          * Il faut donc que "av" < "avenue" soit false
          */
        bool operator()(const std::string & a, const std::pair<std::string, std::vector<T> > & b){
            if(b.first.find(a) == 0) return false;
            return (a < b.first);
        }

        /** Utilisé pour la borne sup. Ici rien d'extraordinaire */
        bool operator()(const std::pair<std::string, std::vector<T> > & b, const std::string & a){
            return (b.first < a);
        }
    };

    /** Retrouve toutes les positions des élements contenant le mot des mots qui commencent par token */
    std::vector<T> match(const std::string & token){
        // Les éléments dans vec_map sont triés par ordre alphabétiques, il suffit donc de trouver la borne inf et sup
        auto lower = std::lower_bound(vec_map.begin(), vec_map.end(), token, comp());
        auto upper = std::upper_bound(vec_map.begin(), vec_map.end(), token, comp());

        std::vector<T> result;

        // On concatène tous les indexes
        // Pour les raisons de perfs mesurées expérimentalement, on accepte des doublons
        for(; lower != upper; ++lower){
            std::vector<T> other = lower->second;
            result.insert(result.end(), other.begin(), other.end());
        }
        return result;
    }

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant tous ces mots*/
    std::vector<T> find(const std::string & str){
        std::vector<T> result;
        boost::tokenizer<> tokens(str);
        auto token_it = tokens.begin();
        if(token_it != tokens.end()){
            // Premier résultat. Il y aura au plus ces indexes
            result = match(*token_it);
            for(++token_it; token_it != tokens.end(); ++token_it){
                std::vector<T> new_result;
                std::sort(result.begin(), result.end());
                BOOST_FOREACH(auto i, match(*token_it)){
                    // Binary search fait une recherche dichotomique pour savoir si l'élément i existe
                    // S'il existe dans les deux cas, on le garde
                    if(binary_search(result.begin(), result.end(), i)){
                        new_result.push_back(i);
                    }
                }
                // std::unique retrie les donnée et fout les doublons à la fin
                // La fonction retourne un itérateur vers le premier doublon
                // Expérimentalement le gain est très faible
                result.assign(new_result.begin(), std::unique(new_result.begin(), new_result.end()));
            }
        }
        return result;
    }
};
