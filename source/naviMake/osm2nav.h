#pragma once
#include <string>
#include "utils/csv.h"

namespace navitia { namespace georef {
class GeoRef;

/** Remplit l'objet georef à partir du fichier openstreetmap */
void fill_from_osm(GeoRef & geo_ref_to_fill, const std::string & osm_pbf_filename);
void fill_from_poi(GeoRef & geo_ref_to_fill, const std::string & poi_dirname);
void fill_poi_type(GeoRef & geo_ref_to_fill , const std::string & poi_dirname);
void fill_poi(GeoRef & geo_ref_to_fill , const std::string & poi_dirname);
}}

