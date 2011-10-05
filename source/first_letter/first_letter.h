#pragma once
#include <boost/algorithm/string/join.hpp>

#include<boost/foreach.hpp>
#include<boost/tokenizer.hpp>
#include<map>
#include<set>
#include<bitset>
struct comp{
    bool operator()(const std::string & a, const std::pair<std::string, std::vector<unsigned int> > & b){
        if(b.first.find(a) == 0) return false;
        return (a < b.first);
    }
};

struct comp2{
    bool operator()(const std::pair<std::string, std::vector<unsigned int> > & b, const std::string & a){
        //if(b.first.find(a) == 0) return false;
        return (b.first < a);
    }
};

typedef unsigned int idx_t;
struct FirstLetter
{
    typedef boost::tokenizer<> Tokenizer;
    std::map<std::string, std::set<idx_t> > map;
    typedef std::pair<std::string, std::vector<idx_t> > vec_elt;
    std::vector<vec_elt> vec_map;

    void add_element(const std::string & token, idx_t position){
        map[token].insert(position);
    }

    void add_string(const std::string & str, idx_t position){
        Tokenizer tokens(str);
        BOOST_FOREACH(auto token, tokens){
            add_element(token, position);
        }
    }

    std::vector<unsigned int> match(const std::string & token){
        auto lower = std::lower_bound(vec_map.begin(), vec_map.end(), token, comp2());
        auto upper = std::upper_bound(vec_map.begin(), vec_map.end(), token, comp());

        std::vector<unsigned int> result;

        for(; lower != upper; ++lower){
            std::vector<unsigned int> other = lower->second;
            result.insert(result.end(), other.begin(), other.end());
        }
     //   std::sort(result.begin(), result.end());

        return result;
    }

    void build(){
        vec_map.reserve(map.size());
        BOOST_FOREACH(auto key_val, map){
            vec_map.push_back(std::make_pair(key_val.first, std::vector<idx_t>(key_val.second.begin(), key_val.second.end())));
        }
    }

    std::vector<idx_t> find(const std::string & str){
        std::vector<idx_t> result;
        Tokenizer tokens(str);
        auto token_it = tokens.begin();
        if(token_it != tokens.end()){
            result = match(*token_it);
            std::sort(result.begin(), result.end());
            for(++token_it; token_it != tokens.end(); ++token_it){
                std::vector<idx_t> tmp = match(*token_it);
                std::vector<idx_t> new_result;
                new_result.reserve(tmp.size());
                BOOST_FOREACH(auto i, tmp){
                    if(binary_search(result.begin(), result.end(), i))
                        new_result.push_back(i);
                }
                std::sort(new_result.begin(), new_result.end());
                result = new_result;
            }
        }
        return result;
    }
};
