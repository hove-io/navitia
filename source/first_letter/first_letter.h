#pragma once

#include<boost/foreach.hpp>
#include<boost/tokenizer.hpp>
#include<map>
#include<set>

struct comp{
    bool operator()(const std::string & a, const std::pair<std::string, std::set<unsigned int> > & b){
    if(a < b.first) return true;
    //if(b.first.find(a) != std::string::npos) return true;
    return false;
}
};

typedef unsigned int idx_t;
struct FirstLetter
{
    typedef boost::tokenizer<> Tokenizer;
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
        auto upper = std::upper_bound(map.begin(), map.end(), token, comp());

        std::set<idx_t> result;
        for(; lower != upper; ++lower){
            result.insert(lower->second.begin(), lower->second.end());
        }

        return result;
    }

    std::set<idx_t> find(const std::string & str){
        Tokenizer tokens(str);
        std::set<idx_t> result = match(*(tokens.begin()));

        BOOST_FOREACH(auto token, tokens){
            std::set<idx_t> tmp = match(token);
            std::set<idx_t> new_result;
            std::insert_iterator< std::set<idx_t> > ii(new_result, new_result.begin());
            std::set_intersection(result.begin(), result.end(), tmp.begin(), tmp.end(), ii);
            result = new_result;
        }
        return result;
    }
};
