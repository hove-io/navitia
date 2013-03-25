#pragma once
#include <unordered_map>
#include "type/type.h"

namespace nt = navitia::type;
namespace navitia {

    namespace georef {
        struct Levels{
            std::map<std::string, std::string> LevelList;
            Levels(){
//                LevelList["2"]="Pays";
//                LevelList["4"]="Région";
//                LevelList["6"]="Département";
                LevelList["8"]="Commune";
//                LevelList["10"]="Quartier";
            }
        };

        struct Admin : nt::NavitiaHeader, nt::Nameable {
            /**
              Level = 2  : Pays
              Level = 4  : Région
              Level = 6  : Département
              Level = 8  : Commune
              Level = 10 : Quartier
            */
            int level;
            std::string post_code;
            std::string insee;
            nt::GeographicalCoord coord;
            std::vector<nt::GeographicalCoord> boundary;            

            Admin():level(-1){}
            Admin(int lev):level(lev){}
            template<class Archive> void serialize(Archive & ar, const unsigned int ) {
                ar & idx & level & post_code & insee & name & idx & external_code & coord ;
            }
        };
/*


        struct AdminRef{
            std::vector<Admin> admin;

            AdminRef(){}
            template<class Archive> void serialize(Archive & ar, const unsigned int ){
                ar & admin;
            }
        };*/
    }
}
