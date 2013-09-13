#pragma once
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <algorithm>
#include <regex>
#include <boost/regex.hpp>



#include <map>
#include <unordered_map>
#include <set>
#include "type/type.h"

namespace navitia { namespace autocomplete {


/** Map de type Autocomplete
  *
  * On associe une chaine de caractères, par exemple "rue jean jaures" à une valeur T (typiquement un pointeur
  * ou un indexe)
  *
  * Quand on cherche "r jean" on veut récupérer la valeur correspondant à la clef "rue jean jaures" et "rue jeanne d'arc"
  */
template<class T>
struct Autocomplete
{
    /// structure qui contient la position des mots dans autocomplete et le nombre de match.
    struct fl_quality{
        T idx;
        int nb_found;
        int word_len;
        int quality;
        navitia::type::GeographicalCoord coord;
        int house_number;

        fl_quality() :nb_found(0), word_len(0), quality(0),house_number(-1) {}
        bool operator<(const fl_quality & other) const{
            return this->quality > other.quality;
        }

    };

    /// Structure temporaire pour garder les informations sur chaque ObjetTC:
    struct word_quality{
        int word_count;
        int word_distance;
        int score;

        word_quality():word_count(0), word_distance(0){}
        template<class Archive> void serialize(Archive & ar, const unsigned int) {
            ar & word_count & word_distance & score;
        }
    };

    /// Structure temporaire pour construire l'indexe
    std::map<std::string, std::set<T> > map;

    /// À chaque mot (par exemple "rue" ou "jaures") on associe un tableau de T qui contient la liste des éléments contenant ce mot
    typedef std::pair<std::string, std::vector<T> > vec_elt;

    /// Structure principale de notre indexe
    std::vector<vec_elt> vec_map;

    /// Structure pour garder les informations comme nombre des mots, la distance des mots...dans chaque Autocomplete (Position)
    std::map<T, word_quality> ac_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & vec_map & ac_list;
    }

    // Méthodes permettant de construire l'indexe

    /** Étant donné une chaîne de caractères et la position de l'élément qui nous intéresse :
      * – on découpe en mots la chaîne (tokens)
      * — on rajoute la position à la liste de chaque mot
      */
    void add_string(std::string str, T position, const std::map<std::string, std::string> & map_alias,
                    const std::map<std::string, std::string> & map_synonymes){
        word_quality wc;
        int distance = 0;

        //Appeler la méthode pour traiter les alias avant de les ajouter dans le dictionaire:
        std::vector<std::string> vec_word = tokenize(str, map_alias, map_synonymes);
        int count = vec_word.size();
        auto vec = vec_word.begin();
        while(vec != vec_word.end()){
            map[*vec].insert(position);
            distance += (*vec).size();
            ++vec;
        }
        wc.word_count = count;
        wc.word_distance = distance;
        ac_list[position] = wc;
    }

    /** Construit la structure finale
      *
      * Les map et les set sont bien pratiques, mais leurs performances sont mauvaises avec des petites données (comme des ints)
      */
    void build(){
        vec_map.reserve(map.size());
        for(auto key_val: map){
            vec_map.push_back(std::make_pair(key_val.first, std::vector<T>(key_val.second.begin(), key_val.second.end())));
        }
    }


    void compute_score(const type::PT_Data &pt_data, const georef::GeoRef &georef,
                       const type::Type_e type);
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
    std::vector<T> find(std::vector<std::string> vecStr) const {
        std::vector<T> result;
        auto vec = vecStr.begin();
        if(vec != vecStr.end()){
            // Premier résultat. Il y aura au plus ces indexes
            result = match(*vec);

            for(++vec; vec != vecStr.end(); ++vec){
                std::vector<T> new_result;
                std::sort(result.begin(), result.end());
                for(auto i : match(*vec)){
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
    std::vector<fl_quality> find_partial(std::string str, const std::map<std::string, std::string> & map_alias, int wordweight) const {
        int wordLength = 0;
        std::unordered_map<T, fl_quality> fl_result;
        boost::to_lower(str);
        str = get_alias(str, map_alias);
        std::vector<T> index_result;

        // Créer un vector de réponse:
        std::vector<fl_quality> vec_quality;
        fl_quality quality;
        wordLength = lettercount(str);


        boost::tokenizer<> tokens(str);
        auto token_it = tokens.begin();
        if(token_it != tokens.end()){
            // Premier résultat. Il y aura au plus ces indexes
            index_result = match(*token_it);

            //incrémenter la propriété "nb_found" pour chaque index des mots autocomplete dans vec_map
            add_word_quality(fl_result,index_result);

            //Recherche des mots qui restent
            for(++token_it; token_it != tokens.end(); ++token_it){
                index_result = match(*token_it);

                //incrémenter la propriété "nb_found" pour chaque index des mots autocomplete dans vec_map
                add_word_quality(fl_result,index_result);
            }

            //remplir le tableau temp_result avec le résultat de qualité.
            for(auto pair : fl_result){
                quality.idx = pair.first;
                quality.nb_found = pair.second.nb_found;
                quality.word_len = wordLength;
                quality.quality = calc_quality(quality, wordweight);
                vec_quality.push_back(quality);
            }

            std::sort(vec_quality.begin(), vec_quality.end());
        }

        return vec_quality;
    }

    std::vector<fl_quality> sort_and_truncate(std::vector<fl_quality> input, size_t nbmax) const {
        typename std::vector<fl_quality>::iterator middle_iterator;
        if(nbmax < input.size())
            middle_iterator = input.begin() + nbmax;
        else
            middle_iterator = input.end();
        std::partial_sort(input.begin(), middle_iterator, input.end());

        if (input.size() > nbmax){input.resize(nbmax);}
        return input;
    }

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant au moins un des mots*/
    std::vector<fl_quality> find_complete(const std::string & str, const std::map<std::string, std::string> & map_alias,
                                          const std::map<std::string, std::string> & map_synonymes,const int wordweight,
                                          size_t nbmax,
                                          std::function<bool(T)> keep_element)
                                          const{
        std::vector<std::string> vec = tokenize(str, map_alias, map_synonymes);
        int wordCount = 0;
        int wordLength = 0;
        fl_quality quality;
        std::vector<T> index_result;
        index_result = find(vec);
        wordCount = vec.size();
        wordLength = wordcount(vec);

        // Créer un vector de réponse:
        std::vector<fl_quality> vec_quality;

        for(auto i : index_result){
            if(keep_element(i)) {
                quality.idx = i;
                quality.nb_found = wordCount;
                quality.word_len = wordLength;
                quality.quality = calc_quality(quality, wordweight);
                vec_quality.push_back(quality);
            }
        }

        return sort_and_truncate(vec_quality, nbmax);
    }


    /** pour chaque mot trouvé dans la liste des mots il faut incrémenter la propriété : nb_found*/
    /** Utilisé que pour une recherche partielle */
    void add_word_quality(std::unordered_map<T, fl_quality> & fl_result, const std::vector<T> &found) const{
        for(auto i : found){
            fl_result[i].nb_found++;
        }
    }

    int calc_quality(const fl_quality & ql,  int wordweight) const {
        int result = 100;

        //Qualité sur le nombres des mot trouvé
        result -= (ac_list.at(ql.idx).word_count - ql.nb_found) * wordweight;//coeff  WordFound

        //Qualité sur la distance globale des mots.
        result -= (ac_list.at(ql.idx).word_distance - ql.word_len);//Coeff de la distance = 1        

        return result;
    }


    int lettercount(const std::string &str) const {
        int result = 0;
        boost::tokenizer <> tokens(str);

        for (auto token_it: tokens){
            result += token_it.size();
        }
        return result;
    }

    int wordcount(std::vector<std::string> & words) const{
        int distance = 0;
        auto vec = words.begin();
        while(vec != words.end()){
            distance += (*vec).size();
            ++vec;
        }
        return distance;
    }

    /** Méthode pour récuperer le mot alias (Ghostword, alias et ShortName sont géré ici)*/
    /** Si le mot cherché n'est pas trouvé alors envoyer le même mot*/
    std::string alias_word(const std::string & str, const std::map<std::string, std::string> & map_alias) const{
        std::map<std::string, std::string>::const_iterator it = map_alias.find(str);
        if (it!= map_alias.end()){
            return it->second;
        }
        else{
            return str;
        }
    }

    // à supprimer plus tard -> utilisé que par find_partial
    std::string get_alias(const std::string & str, const std::map<std::string, std::string> & map_alias) const{
        std::string result = "";
        boost::tokenizer <> tokens(str);
        for (auto token_it: tokens){
            result += " " + alias_word(token_it, map_alias);
        }
        return result;
    }

    std::string strip_accents(std::string str) const {
        std::vector< std::pair<std::string, std::string> > vec_str;
        vec_str.push_back(std::make_pair("à","a"));
        vec_str.push_back(std::make_pair("â","a"));
        vec_str.push_back(std::make_pair("æ","ae"));
        vec_str.push_back(std::make_pair("é","e"));
        vec_str.push_back(std::make_pair("è","e"));
        vec_str.push_back(std::make_pair("ê","e"));
        vec_str.push_back(std::make_pair("ô","o"));
        vec_str.push_back(std::make_pair("û","u"));
        vec_str.push_back(std::make_pair("ù","u"));
        vec_str.push_back(std::make_pair("ç","c"));
        vec_str.push_back(std::make_pair("ï","i"));
        vec_str.push_back(std::make_pair("œ","oe"));

        auto vec = vec_str.begin();
        while(vec != vec_str.end()){
            boost::algorithm::replace_all(str, vec->first, vec->second);
            ++vec;
        }
        return str;
    }

    std::vector<std::string> tokenize(const std::string & str,  const std::map<std::string, std::string> & map_alias,
                                      const std::map<std::string, std::string> & map_synonymes) const{
        std::vector<std::string> vec;
        std::string strToken;
        std::string strFind = str;
        boost::to_lower(strFind);

        //traiter les caractères accentués
        strFind = strip_accents(strFind);

        // Remplacer les synonymes qui existent dans le MAP
        std::map<std::string, std::string>::const_iterator it = map_synonymes.begin();
        while (it != map_synonymes.end()){
            //boost::replace_all(strFind,it->first, it->second);
            strFind = boost::regex_replace(strFind,boost::regex("\\<" + it->first + "\\>"), it->second);
            ++it;
        }

        boost::tokenizer <> tokens(strFind);
        for (auto token_it: tokens){
            strToken = alias_word(token_it, map_alias);
            if (!strToken.empty()){
                vec.push_back(strToken);
            }

        }
        return vec;
    }


    bool is_address_type(const std::string & str,  const std::map<std::string, std::string> & map_alias,
                          const std::map<std::string, std::string> & map_synonymes) const{
        bool result = false;
        std::vector<std::string> vec_token = tokenize(str, map_alias, map_synonymes);
        std::vector<std::string> vecTpye = {"rue", "avenue", "place", "boulevard","chemin", "impasse"};
        auto vtok = vec_token.begin();
        while(vtok != vec_token.end() && (result == false)){
            //Comparer avec le vectorType:
            auto vtype = vecTpye.begin();
            while(vtype != vecTpye.end() && (result == false)){
                if (*vtok == *vtype){
                    result = true;

                }
                ++vtype;
            }
            ++vtok;
        }
        return result;
    }
};

}} // namespace navitia::autocomplete
