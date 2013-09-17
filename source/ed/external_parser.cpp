#include "external_parser.h"
#include <boost/lexical_cast.hpp>
#include "utils/csv.h"
#include "utils/configuration.h"
#include <boost/algorithm/string.hpp>



//namespace navitia{ namespace georef{
namespace ed{ namespace connectors{
ExternalParser::ExternalParser() {
        init_logger();
        logger = log4cplus::Logger::getInstance("log");
}

void ExternalParser::fill_aliases(const std::string &file, Data & data){
    // Verification des entêtes:
    std::string key, value;
    CsvReader csv(file, '=', true);
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
                data.alias[key]=value;
            }
        }
    }
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(data.alias.size())+ " aliases");
}

void ExternalParser::fill_synonyms(const std::string &file, Data & data)
{
    std::string key, value;
    CsvReader csv(file, '=', true);
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
                data.synonymes[key]=value;
            }
        }
    }
    LOG4CPLUS_TRACE(logger, "On a  " + boost::lexical_cast<std::string>(data.synonymes.size())+ " synonymes");
}

}}
