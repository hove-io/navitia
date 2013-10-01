#include "poi_parser.h"
#include "type/data.h"
#include <boost/lexical_cast.hpp>
#include "type/data.h"
#include "georef/georef.h"
#include "utils/csv.h"
#include "utils/configuration.h"
#include <boost/algorithm/string.hpp>



namespace ed{ namespace connectors{
PoiParser::PoiParser(const std::string & path): path(path){
        init_logger();
        logger = log4cplus::Logger::getInstance("log");
}


void PoiParser::fill_poi_type(navitia::georef::GeoRef & georef_to_fill)
{
    // Verification des entêtes:
    CsvReader csv(path + "/" + "poi_type.txt", ',', true);
    if(!csv.is_open()) {
        LOG4CPLUS_WARN(logger, "le fichier " + csv.filename +" n'existe pas ");
        return;
    }
    std::vector<std::string> mandatory_headers = {"poi_type_id" , "poi_type_name"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_ERROR(logger, "Erreur lors du parsing de " + csv.filename +" . Il manque les colonnes : " + csv.missing_headers(mandatory_headers));
    }

    int id_c = csv.get_pos_col("poi_type_id"), name_c = csv.get_pos_col("poi_type_name");

    int id = 1;
    while(!csv.eof()){

        auto row = csv.next();
        if (!row.empty()){
            navitia::georef::POIType* ptype = new navitia::georef::POIType();
            ptype->idx = id-1;
            if (id_c != -1 && row[id_c]!= "")
                ptype->uri = row[id_c];
            else
                ptype->uri = boost::lexical_cast<std::string>(id);
            ptype->name = row[name_c];
            georef_to_fill.poitypes.push_back(ptype);
            georef_to_fill.poitype_map[ptype->uri] = ptype->idx;
            ++id;
        }
    }

    //Chargement de la liste poitype_map
//    georef_to_fill.normalize_extcode_poitypes();
}

void PoiParser::fill_poi(navitia::georef::GeoRef & georef_to_fill)
{
    // Verification des entêtes:
    CsvReader csv(path + "/" + "poi.txt", ',', true);
    if(!csv.is_open()) {
        LOG4CPLUS_WARN(logger, "Le fichier " + csv.filename +" n'existe pas");
        return;
    }

    std::vector<std::string> mandatory_headers = {"poi_id", "poi_name", "poi_lat", "poi_lon"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_ERROR(logger, "Erreur lors du parsing de " + csv.filename +" . Il manque les colonnes : " + csv.missing_headers(mandatory_headers));
    }
    int id_c = csv.get_pos_col("poi_id"), name_c = csv.get_pos_col("poi_name"), weight_c = csv.get_pos_col("poi_weight"),
            visible_c = csv.get_pos_col("poi_visible"),
            lat_c = csv.get_pos_col("poi_lat"), lon_c = csv.get_pos_col("poi_lon"), type_c = csv.get_pos_col("poi_type_id");

    int id = 1;
    while (!csv.eof()){
        auto row = csv.next();
        if (!row.empty()){
            navitia::georef::POI* poi = new navitia::georef::POI();
            poi->idx = id-1;
            if (id_c != -1 && row[id_c] != "")
                poi->uri = row[id_c];
            else
                poi->uri = boost::lexical_cast<std::string>(id);
            poi->name = row[name_c];
            if (weight_c != -1 && row[weight_c] != "")
                poi->weight = boost::lexical_cast<int>(row[weight_c]);
            else
                poi->weight = 0;

            try{
                poi->coord.set_lon(boost::lexical_cast<double>(row[lon_c]));
                poi->coord.set_lat(boost::lexical_cast<double>(row[lat_c]));
            }
            catch(boost::bad_lexical_cast ) {
                std::cout << "Impossible de parser les coordonnées pour "
                          << row[id_c] << " " << row[name_c];
            }

            // Lire la référence du type de POI
            auto ptype = georef_to_fill.poitype_map.find(row[type_c]);
            if (ptype == georef_to_fill.poitype_map.end()){
                // ajouter l'Index par défaut ??
                poi->poitype_idx = -1;
            }
            else
                poi->poitype_idx = ptype->second;

            if (visible_c != -1 && row[visible_c] != "")
                poi->visible = boost::lexical_cast<bool>(row[visible_c]);
            georef_to_fill.pois.push_back(poi);
            ++id;
        }
    }
    //Chargement de la liste poitype_map
//    georef_to_fill.normalize_extcode_pois();

}

void PoiParser::fill(navitia::georef::GeoRef & georef_to_fill)
{
    // Lire les fichiers poi-type.txt
    fill_poi_type(georef_to_fill);
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(georef_to_fill.poitypes.size())+ " POITypes");

    // Lire les fichiers poi.txt
    fill_poi(georef_to_fill);
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(georef_to_fill.pois.size())+ " POIs");
}

void PoiParser::fill_aliases(navitia::georef::GeoRef & georef_to_fill){
    // Verification des entêtes:
    std::string key, value;
    CsvReader csv(path + "/" + "alias.txt", '=', true);
    if(!csv.is_open()) {
        LOG4CPLUS_FATAL(logger, "Impossible d'ouvrir le fichier " + csv.filename +" dans fill_aliases");
        return;
    }
    std::vector<std::string> mandatory_headers = {"key" , "value"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Erreur lors du parsing de fill_aliases " + csv.filename +" . Il manque les colonnes : " + csv.missing_headers(mandatory_headers));
    }

    int key_c = csv.get_pos_col("key"), value_c = csv.get_pos_col("value");
    while(!csv.eof()){
        auto row = csv.next();
        if (!row.empty()){
            if (key_c != -1){
                key = row[key_c];
                value = row[value_c];
                boost::to_lower(key);
                boost::to_lower(value);
                georef_to_fill.alias[key]= value;
            }
        }
    }
}

void PoiParser::fill_synonyms(navitia::georef::GeoRef &georef_to_fill)
{
    std::string key, value;
    CsvReader csv(path + "/" + "synonyme.txt", '=', true);
    if(!csv.is_open()) {
        LOG4CPLUS_FATAL(logger, "Impossible d'ouvrir le fichier " + csv.filename +" dans fill_synonyms");
        return;
    }
    std::vector<std::string> mandatory_headers = {"key" , "value"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Erreur lors du parsing de " + csv.filename +" . Il manque les colonnes : " + csv.missing_headers(mandatory_headers));
    }

    int key_c = csv.get_pos_col("key"), value_c = csv.get_pos_col("value");
    while(!csv.eof()){
        auto row = csv.next();
        if (!row.empty()){
            if (key_c != -1){
                key = row[key_c];
                value = row[value_c];
                boost::to_lower(key);
                boost::to_lower(value);
                georef_to_fill.synonymes[key]=value;
            }
        }
    }
}

void PoiParser::fill_alias_synonyme(navitia::georef::GeoRef &georef_to_fill)
{
    // Lire les fichiers poi-type.txt
    fill_aliases(georef_to_fill);
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(georef_to_fill.alias.size())+ " alias");

    // Lire les fichiers poi-type.txt
    fill_synonyms(georef_to_fill);
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(georef_to_fill.synonymes.size())+ " synonymes");
}


}}
