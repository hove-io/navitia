#pragma once
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <algorithm>

#include <map>
#include <unordered_map>
#include <set>

namespace navitia { namespace firstletter {


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
    /// structure qui contient la position des mots dans firstletter et le nombre de match.
    struct fl_quality{
        T idx;
        int nb_found;
        int quality;

        fl_quality(){
            nb_found = 0;
            quality = 0;
        }

        bool operator<(const fl_quality & other) const{
            return this->quality > other.quality;
        }

    };


    /// Structure temporaire pour construire l'indexe
    std::map<std::string, std::set<T> > map;

    /// À chaque mot (par exemple "rue" ou "jaures") on associe un tableau de T qui contient la liste des éléments contenant ce mot
    typedef std::pair<std::string, std::vector<T> > vec_elt;

    /// Structure principale de notre indexe
    std::vector<vec_elt> vec_map;

    /// Structure pour garder le nombre des mots dans chaque FirstLetter (Position)
    std::map<T, int> word_count;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & vec_map &word_count;
    }

    // Méthodes permettant de construire l'indexe

    /** Étant donné une chaîne de caractères et la position de l'élément qui nous intéresse :
      * – on découpe en mots la chaîne (tokens)
      * — on rajoute la position à la liste de chaque mot
      */
    void add_string(std::string str, T position){
        int count = 0;
        boost::to_lower(str);
        boost::tokenizer<> tokens(str);
        BOOST_FOREACH(auto token, tokens){
            map[token].insert(position);
            count++;
        }
        word_count[position] = count;
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
    std::vector<T> match(const std::string &token) const {
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
    std::vector<T> find(std::string str) const {
        boost::to_lower(str);
        std::vector<T> result;
        boost::tokenizer<> tokens(str);
        auto token_it = tokens.begin();
        if(token_it != tokens.end()){
            // Premier résultat. Il y aura au plus ces indexes
            result = match(*token_it);

            for(++token_it; token_it != tokens.end(); ++token_it){
                std::vector<T> new_result;
                std::sort(result.begin(), result.end());                
                BOOST_FOREACH(auto i,match(*token_it)){
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

    /** Définit un fonctor permettant de parcourir notqualityre structure un peu particulière : trier par la valeur "nb_found"*/
    /** associé au valeur du vector<T> */
    struct Compare{
        const std::unordered_map<T, fl_quality>  & fl_result;// Pointeur au fl_result
        Compare(const std::unordered_map<T, fl_quality>  & fl_result) : fl_result(fl_result) {} //initialisation

        bool operator()(T a, T b) const{
            return fl_result.at(a).nb_found > fl_result.at(b).nb_found;
        }
    };

    /** Définit un fonctor permettant de parcourir notre structure un peu particulière : trier par la valeur "nb_found"*/
    /** associé au valeur du vector<T> */
    struct compare_by_quality{
        const std::unordered_map<T, fl_quality> & fl_result;
        compare_by_quality(const std::unordered_map<T, fl_quality>  & fl_result) : fl_result(fl_result) {} //initialisation

        bool operator()(T a, T b) const{
            return fl_result.at(a).quality > fl_result.at(b).quality;
        }

    };

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant au moins un des mots*/
    std::vector<fl_quality> find_partial(std::string str) const {
        int wordCount = 0;
        std::unordered_map<T, fl_quality> fl_result;
        boost::to_lower(str);
        std::vector<T> index_result;

        // Créer un vector de réponse:
        std::vector<fl_quality> vec_quality;
        fl_quality quality;

        boost::tokenizer<> tokens(str);
        auto token_it = tokens.begin();
        if(token_it != tokens.end()){
            // Premier résultat. Il y aura au plus ces indexes
            index_result = match(*token_it);
            wordCount++;

            //incrémenter la propriété "nb_found" pour chaque index des mots firstletter dans vec_map
            add_word_quality(fl_result,index_result);

            //Recherche des mots qui restent
            for(++token_it; token_it != tokens.end(); ++token_it){
                wordCount++;
                index_result = match(*token_it);

                //incrémenter la propriété "nb_found" pour chaque index des mots firstletter dans vec_map
                add_word_quality(fl_result,index_result);
            }

            //remplir le tableau temp_result avec le résultat de qualité.
            BOOST_FOREACH(auto pair, fl_result){
                quality.idx = pair.first;
                quality.nb_found = pair.second.nb_found;
                quality.quality = calc_quality(quality);
                vec_quality.push_back(quality);
            }

            std::sort(vec_quality.begin(), vec_quality.end());
        }

        return vec_quality;
    }

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant au moins un des mots*/
    std::vector<fl_quality> find_complete(const std::string & str) const{
        int wordCount = 0;
        fl_quality quality;
        std::vector<T> index_result;
        index_result = find(str);
        wordCount = count(str);

        // Créer un vector de réponse:
        std::vector<fl_quality> vec_quality;

        BOOST_FOREACH(auto i, index_result){
            quality.idx = i;
            quality.nb_found = wordCount;
            quality.quality = calc_quality(quality);
            vec_quality.push_back(quality);
        }

        std::sort(vec_quality.begin(), vec_quality.end());

        return vec_quality;
    }


    /** pour chaque mot trouvé dans la liste des mots il faut incrémenter la propriété : nb_found*/
    /** Utilisé que pour une recherche partielle */
    void add_word_quality(std::unordered_map<T, fl_quality> & fl_result, const std::vector<T> &found) const{
        BOOST_FOREACH(auto i, found){
            fl_result[i].nb_found++;

        }
    }

    int calc_quality(const fl_quality & ql) const {
        return ql.nb_found * 100 / std::max(word_count.at(ql.idx), 1);
    }

    int count(const std::string &str) const {
        int result = 0;
        boost::tokenizer <> tokens(str);

        for (auto token_it = tokens.begin(); token_it != tokens.end(); ++token_it){
            ++result;
        }
        return result;
    }

    /** Méthode pour récuperer le mot alias (Ghostword, alias et ShortName sont géré ici)*/
    /** Si le mot cherché n'est pas trouvé alors envoyer le même mot*/
    std::string get_alias(const std::string & str, const std::map<std::string, std::string> & map_alias) const{
        std::map<std::string, std::string>::const_iterator it = map_alias.find(str);
        if (it!= map_alias.end()){
            return it->second;
        }
        else{
            return str;
        }
    }
};

}} // namespace navitia::firstletter
