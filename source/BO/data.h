#pragma once
#include "types.h"
#include <boost/foreach.hpp>
namespace BO{

    class Data{
    public:
        std::vector<BO::types::Network*> networks;
        std::vector<BO::types::ModeType*> mode_types;
        std::vector<BO::types::Line*> lines;
        std::vector<BO::types::Mode*> modes;


        ~Data(){
            BOOST_FOREACH(BO::types::Network* network, networks){
                delete network;
            }
            BOOST_FOREACH(BO::types::ModeType* mode_type, mode_types){
                delete mode_type;
            }
            BOOST_FOREACH(BO::types::Line* line, lines){
                delete line;
            }
            BOOST_FOREACH(BO::types::Mode* mode, modes){
                delete mode;
            }
        }

    };
}
