#include "baseworker.h"


#include <iostream>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
#include <set>
#include <string>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/assign/std/set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
typedef boost::tokenizer<> Tokenizer;

using namespace std;
using namespace __gnu_pbds;
using namespace boost::assign;
using namespace webservice;

typedef std::vector<string*>		mapped_type;
typedef string_trie_e_access_traits<> 	cmp_fn;
typedef pat_trie_tag 			tag_type;

typedef trie<string, mapped_type, cmp_fn, tag_type,  trie_prefix_search_node_update> trie_type;

mapped_type match(const trie_type & t, const std::string & key) {
    mapped_type ret;
    ret.reserve(1000);
    auto range = t.prefix_range(key);
    for (auto it = range.first; it != range.second; ++it){
        ret.insert(ret.end(), it->second.begin(), it->second.end());
    }
    return ret;
}

mapped_type match_all(const trie_type & t, const std::string & str){
    mapped_type ret;
    if(str == "")
        return ret;

    Tokenizer tokens(str);
    auto it = tokens.begin();
    ret = match(t, *it);
    ++it;
    for(; it != tokens.end(); it++) {
        mapped_type new_ret;
        new_ret.reserve(ret.size());
        mapped_type tmp = match(t, *it);
        insert_iterator<mapped_type> ii(new_ret, new_ret.begin());
        std::set_intersection(ret.begin(), ret.end(), tmp.begin(), tmp.end(), ii);
        ret = new_ret;
        if(ret.size() == 0)
            return ret;
    }
    return ret;
}

typedef unsigned int idx_t;
struct FirstLetter
{
    std::map<std::string, std::set<idx_t> > map;

    void add_element(const std::string & token, idx_t position){
        map[token].insert(position);
    }

    void add_string(const std::string & str, idx_t position){
        Tokenizer tokens(str);
        BOOST_FOREACH(auto token, tokens){
            add_element(token, position);
        }
    }

    std::set<idx_t> match(const std::string & token){
        auto lower = map.lower_bound(token);
        auto upper = map.upper_bound(token);

        std::set<idx_t> result;
        for(; lower != upper; ++lower){
            auto & elts = *lower;
            result.insert(elts.second.begin(), elts.second.end());
        }

        return result;
    }

    std::set<idx_t> find(const std::string & str){
        Tokenizer tokens(str);
        std::set<idx_t> result = match(*(tokens.begin()));

        BOOST_FOREACH(auto token, tokens){
            std::set<idx_t> tmp = match(token);
            std::set<idx_t> new_result;
            insert_iterator< std::set<idx_t> > ii(new_result, new_result.begin());
            std::set_intersection(result.begin(), result.end(), tmp.begin(), tmp.end(), ii);
            result = new_result;
        }
        return result;
    }
};

struct Data{
    int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
    /// Constructeur par défaut, il est appelé au chargement du webservice
    trie_type t;
    std::vector<std::string> adresses;
    Data() : nb_threads(8) {
        adresses.reserve(850000);
        std::fstream ifile("/home/tristram/adresses_uniq.txt");
        std::string line;
        while(getline(ifile, line)) {
            adresses.push_back(boost::to_lower_copy(line));
        }
        std::cout << "Nombre d'adresses : " << adresses.size() << std::endl;

        BOOST_FOREACH(auto & adresse, adresses) {
            Tokenizer tokens(adresse);
            BOOST_FOREACH(auto token, tokens){
                trie_type::iterator it;
                bool b;
                boost::tie(it, b) = t.insert(std::make_pair(token, mapped_type()));
                it->second.push_back(&adresse);
            }
        }
        std::cout << "Ready!" << std::endl;
    }
};

/// Classe associée à chaque thread
class Worker : public BaseWorker<Data> {

    /** Api qui compte le nombre de fois qu'elle a été appelée */
    ResponseData first_letter(RequestData req, Data & d) {
        ResponseData rd;
        auto result = match_all(d.t, boost::to_lower_copy(boost::replace_all_copy(req.params["q"], "%20", " ")));
        BOOST_FOREACH(auto el, result){
            rd.response << *el << std::endl;
        }

        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }



    public:    
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(Data &) {
        register_api("/first_letter",boost::bind(&Worker::first_letter, this, _1, _2), "Cherche les adresses de France dont les tokens commencent par certaines lettres");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
/*int main(int, char**) {
    Data d;
    Worker w(d);
    RequestData rd1;
    rd1.params["q"] = "rue jaur";
    rd1.path ="/first_letter";
    RequestData rd2;
    rd2.params["q"] = "av char de gau";
    rd2.path ="/first_letter";
    RequestData rd3;
    rd3.params["q"] = "poniat";
    rd3.path ="/first_letter";

    
    boost::posix_time::ptime start(boost::posix_time::microsec_clock::local_time());
    for(int i=0; i <1000; i++){
         w(rd1, d);
         w(rd2, d);
         w(rd3, d);
    }
    int duration = (boost::posix_time::microsec_clock::local_time() - start).total_milliseconds();
    std::cout << "Durée totale : " << duration << " ms" << std::endl;
}*/
