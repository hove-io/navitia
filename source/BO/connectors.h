#pragma once

#include <string>
#include "data.h"

namespace BO{ namespace connectors{

struct BadFormated : public std::exception{};

class CsvFusio {
    public:
        CsvFusio(const std::string & path);
        void fill(BO::Data& data);
    private:
        std::string path;


        template <class T>
        T* find(const std::map<int, T*>& items_map, int id){
            typename std::map<int, T*>::const_iterator items_it;
            items_it = items_map.find(id);
            if(items_it != items_map.end()){
                return items_it->second;
            }
            return NULL;
        }

        std::map<int, BO::types::Network*> network_map;
        std::map<int, BO::types::ModeType*> mode_type_map;
        std::map<int, BO::types::Mode*> mode_map;

        void fill_networks(BO::Data& data);
        void fill_modes_type(BO::Data& data);
        void fill_modes(BO::Data& data);
        void fill_lines(BO::Data& data);

};


}}// BO::connectors


