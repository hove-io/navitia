#pragma once

#include <string>
#include "ed/data.h"

namespace ed{ namespace connectors{

using navitia::type::idx_t;


class BDTopoParser {
    public:
        BDTopoParser(const std::string & path);

        /**
         * Charge les Cities dans un objet Data de ed
         *
         */
        void load_city(ed::Data& data);


        /**
         * Charge les données du filaire de voirie dans un objet Data de NAViTiA /!\
         *
         */
        //void load_streetnetwork(navitia::streetnetwork::StreetNetwork & street_network);
        void load_georef(navitia::georef::GeoRef & geo_ref);

    private:
        std::string path;
};


}}// ed::connectors


