#pragma once

#include <string>
#include "data.h"

namespace navimake{ namespace connectors{

using navitia::type::idx_t;


class BDTopoParser {
    public:
        BDTopoParser(const std::string & path);
        
        /**
         * Charge les Cities dans un objet Data de Navimake
         * 
         */
        void load_city(navimake::Data& data);

        
        /**
         * Charge les données du filaire de voirie dans un objet Data de NAViTiA /!\
         *
         */
        //void load_streetnetwork(navitia::streetnetwork::StreetNetwork & street_network);
        void load_georef(navitia::georef::GeoRef & geo_ref);

    private:
        std::string path;
};


}}// navimake::connectors


