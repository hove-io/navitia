#pragma once
#include "type/type.h"

namespace nt = navitia::type;

namespace navitia {
    namespace georef {
        // la liste des pois à charger
        struct Pois{
            std::string poi_key;
            std::string vls;
                    // Key,          Value
            std::map<std::string, std::string> PoiList;
            Pois():poi_key("amenity"), vls("bicycle_rental"){
                PoiList["college"] = "école";
                PoiList["university"] = "université";
                PoiList["theatre"] = "théâtre";
                PoiList["hospital"] = "hôpital";
                PoiList["post_office"] = "bureau de poste";
                PoiList["bicycle_rental"] = "VLS";
            }
        };
}
}

