#pragma once
#include <string>

namespace navitia { namespace georef {
class GeoRef;

/** Remplit l'objet georef à partir du fichier openstreetmap */
void fill_from_osm(GeoRef & geo_ref_to_fill, const std::string & osm_pbf_filename);
}}

