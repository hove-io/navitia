/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "type/type_interfaces.h"
#include "type/geographical_coord.h"
#include "type/fwd_type.h"
#include "utils/functions.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>

#include <algorithm>
#include <boost/regex.hpp>
#include <map>
#include <unordered_map>
#include <set>

namespace navitia {
namespace autocomplete {

struct Compare {
    bool operator()(const std::string& str_a, const std::string& str_b) const {
        if (str_a.length() == str_b.length()) {
            return str_a >= str_b;
        } else {
            return (str_a.length() > str_b.length());
        }
    }
};

std::pair<size_t, size_t> longest_common_substring(const std::string&, const std::string&);

using autocomplete_map = std::map<std::string, std::string, Compare>;
/** Map de type Autocomplete
 *
 * On associe une chaine de caractères, par exemple "rue jean jaures" à une valeur T (typiquement un pointeur
 * ou un indexe)
 *
 * Quand on cherche "r jean" on veut récupérer la valeur correspondant à la clef "rue jean jaures" et "rue jeanne d'arc"
 */
template <class T>
struct Autocomplete {
    /// struct that contains the position of the word in the autocomplete and the number of match
    struct fl_quality {
        T idx = 0;
        int nb_found = 0;
        int word_len = 0;
        int quality = 0;
        navitia::type::GeographicalCoord coord;
        int house_number = -1;
        std::tuple<int, size_t, int> scores = std::make_tuple(0, size_t(0), 0);
        bool operator<(const fl_quality& other) const { return this->quality > other.quality; }
    };

    /// Structure temporaire pour garder les informations sur chaque ObjetTC:
    struct word_quality {
        int word_count;
        int word_distance;
        int score;

        word_quality() : word_count(0), word_distance(0), score(0) {}
        template <class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& word_count& word_distance& score;
        }
    };

    Autocomplete(navitia::type::Type_e otype) : object_type(otype) {}
    Autocomplete() {}  // NOLINT

    /// Type of object
    navitia::type::Type_e object_type;

    /// Structure temporaire pour construire l'indexe
    std::map<std::string, std::set<T> > temp_word_map;

    /// À chaque mot (par exemple "rue" ou "jaures") on associe un tableau de T qui contient la liste des éléments
    /// contenant ce mot
    typedef std::pair<std::string, std::vector<T> > vec_elt;

    /// Structure principale de notre indexe
    std::vector<vec_elt> word_dictionnary;

    /// Structure temporaire pour garder les patterns et leurs indexs
    std::map<std::string, std::set<T> > temp_pattern_map;
    std::vector<vec_elt> pattern_dictionnary;

    /// Structure pour garder les informations comme nombre des mots, la distance des mots...dans chaque Autocomplete
    /// (Position)
    std::map<T, word_quality> word_quality_list;

    // for each T, we store the originaly indexed string (for better score handling)
    std::map<T, std::string> indexed_string;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& word_dictionnary& word_quality_list& pattern_dictionnary& object_type& indexed_string;
    }

    /// Efface les structures de données sérialisées
    void clear() {
        temp_word_map.clear();
        word_dictionnary.clear();
        temp_pattern_map.clear();
        pattern_dictionnary.clear();
        word_quality_list.clear();
        indexed_string.clear();
    }

    // Méthodes permettant de construire l'indexe

    /** Étant donné une chaîne de caractères et la position de l'élément qui nous intéresse :
     * – on découpe en mots la chaîne (tokens)
     * — on rajoute la position à la liste de chaque mot
     */
    void add_string(std::string str,
                    T position,
                    const std::set<std::string>& ghostwords,
                    const autocomplete_map& synonyms) {
        word_quality wc;
        int distance = 0;

        // Appeler la méthode pour traiter les synonymes avant de les ajouter dans le dictionaire:
        auto vec_word = tokenize(str, ghostwords, synonyms);
        // créer des patterns pour chaque mot et les ajouter dans temp_pattern_map:
        add_vec_pattern(vec_word, position);

        int count = vec_word.size();
        auto vec = vec_word.begin();
        while (vec != vec_word.end()) {
            temp_word_map[*vec].insert(position);
            distance += (*vec).size();
            ++vec;
        }
        wc.word_count = count;
        wc.word_distance = distance;
        wc.score = 0;
        word_quality_list[position] = wc;
        indexed_string[position] = strip_accents_and_lower(str);
    }

    void add_vec_pattern(const std::set<std::string>& vec_words, T position) {
        // Créer les patterns:
        std::vector<std::string> vec_patt = make_vec_pattern(vec_words, 2);
        auto v_patt = vec_patt.begin();
        while (v_patt != vec_patt.end()) {
            temp_pattern_map[*v_patt].insert(position);
            ++v_patt;
        }
    }

    // Example of 2-gram : bateau :> ba, at, te, ea, au
    // Example of 3-gram : bateau :> bat, ate, tea, eau
    std::vector<std::string> make_vec_pattern(const std::set<std::string>& vec_words, size_t n_gram) const {
        std::vector<std::string> pattern;
        auto vec = vec_words.begin();
        while (vec != vec_words.end()) {
            std::string word = *vec;
            if (word.length() > n_gram) {
                for (size_t i = 0; i <= word.length() - n_gram; ++i) {
                    pattern.push_back(word.substr(i, n_gram));
                }
            } else {
                pattern.push_back(word);
            }
            ++vec;
        }
        return pattern;
    }

    /** Construit la structure finale
     *
     * Les map et les set sont bien pratiques, mais leurs performances sont mauvaises avec des petites données (comme
     * des ints)
     */
    void build() {
        word_dictionnary.reserve(temp_word_map.size());
        for (auto key_val : temp_word_map) {
            word_dictionnary.push_back(
                std::make_pair(key_val.first, std::vector<T>(key_val.second.begin(), key_val.second.end())));
        }

        // Dictionnaire des patterns:
        pattern_dictionnary.reserve((temp_pattern_map.size()));
        for (auto key_val : temp_pattern_map) {
            pattern_dictionnary.push_back(
                std::make_pair(key_val.first, std::vector<T>(key_val.second.begin(), key_val.second.end())));
        }
    }

    // Méthode pour calculer le score de chaque élément par son admin.
    void compute_score(type::PT_Data& pt_data, georef::GeoRef& georef, const type::Type_e type);
    // Méthodes premettant de retrouver nos éléments
    /** Définit un fonctor permettant de parcourir notre structure un peu particulière */
    struct comp {
        /** Utilisé pour trouver la borne inf. Quand on cherche av, on veux que avenue soit également trouvé
         * Il faut donc que "av" < "avenue" soit false
         */
        bool operator()(const std::string& a, const std::pair<std::string, std::vector<T> >& b) {
            if (b.first.find(a) == 0)
                return false;
            return (a < b.first);
        }

        /** Utilisé pour la borne sup. Ici rien d'extraordinaire */
        bool operator()(const std::pair<std::string, std::vector<T> >& b, const std::string& a) {
            return (b.first < a);
        }
    };

    /** Retrouve toutes les positions des élements contenant le mot des mots qui commencent par token */
    std::vector<T> match(const std::string& token, const std::vector<vec_elt>& vec_source) const {
        // Les éléments dans vec_map sont triés par ordre alphabétiques, il suffit donc de trouver la borne inf et sup
        auto lower = std::lower_bound(vec_source.begin(), vec_source.end(), token, comp());
        auto upper = std::upper_bound(vec_source.begin(), vec_source.end(), token, comp());

        std::vector<T> result;

        // On concatène tous les indexes
        // Pour les raisons de perfs mesurées expérimentalement, on accepte des doublons
        for (; lower != upper; ++lower) {
            std::vector<T> other = lower->second;
            result.insert(result.end(), other.begin(), other.end());
        }
        return result;
    }

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant tous ces mots*/
    std::vector<T> find(const std::set<std::string>& vecStr) const {
        std::vector<T> result;
        auto vec = vecStr.begin();
        if (vec != vecStr.end()) {
            // Premier résultat. Il y aura au plus ces indexes
            result = match(*vec, word_dictionnary);

            // If there is only one word to search we have to sort and delete duplicate results
            if (vecStr.size() == 1) {
                std::sort(result.begin(), result.end());
                result.erase(unique(result.begin(), result.end()), result.end());
            }

            for (++vec; vec != vecStr.end(); ++vec) {
                std::vector<T> new_result;
                std::sort(result.begin(), result.end());
                for (auto i : match(*vec, word_dictionnary)) {
                    // Binary search fait une recherche dichotomique pour savoir si l'élément i existe
                    // S'il existe dans les deux cas, on le garde
                    if (binary_search(result.begin(), result.end(), i)) {
                        new_result.push_back(i);
                    }
                }
                // The function "unique" works only if the vector new_result is sorted.
                std::sort(new_result.begin(), new_result.end());

                // std::unique retrie les donnée et fout les doublons à la fin
                // La fonction retourne un itérateur vers le premier doublon
                // Expérimentalement le gain est très faible
                result.assign(new_result.begin(), std::unique(new_result.begin(), new_result.end()));
            }
        }
        return result;
    }

    /** Définit un fonctor permettant de parcourir notqualityre structure un peu particulière : trier par la valeur
     * "nb_found"*/
    /** associé au valeur du vector<T> */
    struct Compare {
        const std::unordered_map<T, fl_quality>& fl_result;                                    // Pointeur au fl_result
        Compare(const std::unordered_map<T, fl_quality>& fl_result) : fl_result(fl_result) {}  // initialisation

        bool operator()(T a, T b) const { return fl_result.at(a).nb_found > fl_result.at(b).nb_found; }
    };

    /** Définit un fonctor permettant de parcourir notre structure un peu particulière : trier par la valeur
     * "nb_found"*/
    /** associé au valeur du vector<T> */
    struct compare_by_quality {
        const std::unordered_map<T, fl_quality>& fl_result;
        compare_by_quality(const std::unordered_map<T, fl_quality>& fl_result)
            : fl_result(fl_result) {}  // initialisation

        bool operator()(T a, T b) const { return fl_result.at(a).quality > fl_result.at(b).quality; }
    };

    std::vector<fl_quality> sort_and_truncate_by_quality(std::vector<fl_quality> input, size_t nbmax) const {
        sort_and_truncate(input, nbmax, [](const fl_quality& a, const fl_quality& b) { return a.quality > b.quality; });
        return input;
    }

    /**
     * compute the scores of a result
     *
     * the score is:
     *  - the absolute score of the object (like the size of the admin, the number of stoppoints, ...)
     *  - the length of the longuest common substring beetween the string to search and the indexed string
     *  - the position of this substring in the indexed string
     *      this position is used because sometime we have unwanted token far in the indexed string
     *     (like "Busval d'Oise Bus 95-01 (Zone Aéroportuaire Aéroport Charles de Gaulle 1 RER B)"
     *     that will match the 'RER B' query, but is less relevant that the "RER B" object :)
     *
     * the scores are compared lexicographicaly
     * @param str: string to search
     * @param position: element to score
     */
    std::tuple<int, size_t, int> compute_result_scores(const std::string& str, T position) const {
        auto global_score = word_quality_list.at(position).score;

        const auto& indexed_str = indexed_string.at(position);
        auto lcs_and_pos = longest_common_substring(strip_accents_and_lower(str), indexed_str);

        return std::make_tuple(global_score, lcs_and_pos.first,
                               -1 * lcs_and_pos.second  // we want to minimize the position
        );
    }

    /** On passe une chaîne de charactère contenant des mots et on trouve toutes les positions contenant au moins un des
     * mots*/
    std::vector<fl_quality> find_complete(const std::string& str,
                                          size_t nbmax,
                                          std::function<bool(T)> keep_element,
                                          const std::set<std::string>& ghostwords) const {
        auto vec = tokenize(str, ghostwords);
        int wordLength = 0;
        fl_quality quality;
        std::vector<T> index_result;
        // Vector des ObjetTC index trouvés
        index_result = find(vec);
        wordLength = words_length(vec);

        // Créer un vector de réponse:
        std::vector<fl_quality> vec_quality;

        for (auto i : index_result) {
            if (keep_element(i)) {
                quality.idx = i;
                quality.nb_found = word_quality_list.at(quality.idx).word_count;
                quality.word_len = wordLength;
                quality.scores = this->compute_result_scores(str, quality.idx);

                quality.quality = 100;
                vec_quality.push_back(quality);
            }
        }

        sort_and_truncate(vec_quality, nbmax,
                          [](const fl_quality& a, const fl_quality& b) { return a.scores > b.scores; });
        return vec_quality;
    }

    std::vector<fl_quality> compute_vec_quality(const std::string& str,
                                                const std::vector<T>& index_result,
                                                const navitia::georef::GeoRef& geo_ref,
                                                std::function<bool(T)> keep_element,
                                                int word_length) const;

    /** Recherche des patterns les plus proche : faute de frappe */
    std::vector<fl_quality> find_partial_with_pattern(const std::string& str,
                                                      const int word_weight,
                                                      size_t nbmax,
                                                      std::function<bool(T)> keep_element,
                                                      const std::set<std::string>& ghostwords) const {
        // Map temporaire pour garder les patterns trouvé:
        std::unordered_map<T, fl_quality> fl_result;

        // Vector temporaire des indexs
        std::vector<T> index_result;

        // Créer un vector de réponse
        std::vector<fl_quality> vec_quality;
        fl_quality quality;

        auto vec_word = tokenize(str, ghostwords);
        std::vector<std::string> vec_pattern = make_vec_pattern(vec_word, 2);  // 2-grams
        int wordLength = words_length(vec_word);
        int pattern_count = vec_pattern.size();

        // recherche pour le premier pattern:
        auto vec = vec_pattern.begin();
        if (vec != vec_pattern.end()) {
            // Premier résultat:
            index_result = match(*vec, pattern_dictionnary);

            // Incrémenter la propriété "nb_found" pour chaque index des mots autocomplete dans vec_map
            add_word_quality(fl_result, index_result);

            // Recherche des mots qui restent
            for (++vec; vec != vec_pattern.end(); ++vec) {
                index_result = match(*vec, pattern_dictionnary);

                // For each match of n-gram pattern word 1 is added to "nb_found"
                add_word_quality(fl_result, index_result);
            }

            // Compute de highest score of objects found
            int max_score = 0;
            for (auto ir : index_result) {
                if (keep_element(ir)) {
                    max_score = word_quality_list.at(ir).score > max_score ? word_quality_list.at(ir).score : max_score;
                }
            }

            // Here we keep object with match of patternized words >= 75%
            for (auto pair : fl_result) {
                if (keep_element(pair.first)
                    && (((pattern_count - pair.second.nb_found) * 100) / pattern_count <= 25)) {
                    quality.idx = pair.first;
                    quality.nb_found = pair.second.nb_found;
                    quality.word_len = wordLength;
                    quality.scores = this->compute_result_scores(str, quality.idx);
                    quality.quality = calc_quality_pattern(quality, word_weight, max_score, pattern_count);
                    vec_quality.push_back(quality);
                }
            }
        }
        return sort_and_truncate_by_quality(vec_quality, nbmax);
    }

    /** pour chaque mot trouvé dans la liste des mots il faut incrémenter la propriété : nb_found*/
    /** Utilisé que pour une recherche partielle */
    void add_word_quality(std::unordered_map<T, fl_quality>& fl_result, const std::vector<T>& found) const {
        for (auto i : found) {
            fl_result[i].nb_found++;
        }
    }

    int calc_quality_pattern(const fl_quality& ql, int wordweight, int max_score, int patt_count) const {
        int result = 100;

        // Qualité sur le nombres des mot trouvé
        result -= (patt_count - ql.nb_found) * wordweight;  // coeff  WordFound

        // Qualité sur la distance globale des mots.
        result -= abs(word_quality_list.at(ql.idx).word_distance - ql.word_len);  // Coeff de la distance = 1

        // Qualité sur le score
        result -= (max_score - word_quality_list.at(ql.idx).score) / 10;
        return result;
    }

    int words_length(std::set<std::string>& words) const {
        int distance = 0;
        auto vec = words.begin();
        while (vec != words.end()) {
            distance += (*vec).size();
            ++vec;
        }
        return distance;
    }

    std::set<std::string> tokenize(std::string strFind,
                                   const std::set<std::string>& ghostwords,
                                   const autocomplete_map& synonyms = autocomplete_map()) const {
        std::set<std::string> vec;
        boost::to_lower(strFind);
        strFind = boost::regex_replace(strFind, boost::regex("( ){2,}"), " ");

        // traiter les caractères accentués
        strFind = strip_accents(strFind);
        std::string strTemp = strFind;

        // if synonyms contains something, add all synonyms if found while serching on key and value.
        // For each synonyms.key found in strFind add synonyms.value
        for (const auto& it : synonyms) {
            if (boost::regex_search(strFind, boost::regex("\\<" + strip_accents_and_lower(it.first) + "\\>"))) {
                strTemp += " " + strip_accents_and_lower(it.second);
            }
        }

        // For each synonyms.value found in strFind add synonyms.key
        for (const auto& it : synonyms) {
            if (boost::regex_search(strFind, boost::regex("\\<" + strip_accents_and_lower(it.second) + "\\>"))) {
                strTemp += " " + strip_accents_and_lower(it.first);
            }
        }

        // We discard the GhostWords like de, le, la configured in ghostwords as below
        // de,
        // le,
        // la,
        // For each word in ghostwords found in strTemp delete it from strTemp
        for (const auto& it : ghostwords) {
            strTemp = boost::regex_replace(strTemp, boost::regex("\\<" + it + "\\>"), "");
        }

        boost::tokenizer<> tokens(strTemp);
        for (auto token_it : tokens) {
            if (!token_it.empty()) {
                vec.insert(token_it);
            }
        }
        return vec;
    }
};

extern template struct Autocomplete<navitia::type::idx_t>;

}  // namespace autocomplete
}  // namespace navitia
