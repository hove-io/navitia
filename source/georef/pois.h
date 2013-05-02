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

        /** Nommage d'un POI (point of interest). **/
        struct PoiType : public nt::Nameable, nt::Header{
        public:
            template<class Archive> void serialize(Archive & ar, const unsigned int) {
                ar &idx &uri &name;
            }
        private:
        };

        /** Nommage d'un POI (point of interest). **/
        struct Poi : public nt::Nameable, nt::Header{
        public:
            int weight;
            nt::GeographicalCoord coord;
            std::vector<nt::idx_t> admin_list;
            nt::idx_t poitype_idx;

            Poi(): weight(0), poitype_idx(type::invalid_idx){}

            template<class Archive> void serialize(Archive & ar, const unsigned int) {
                ar &idx & uri & idx &name & weight & coord & admin_list & poitype_idx;
            }

        private:
        };
}
}

