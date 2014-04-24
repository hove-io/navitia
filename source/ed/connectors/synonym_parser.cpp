#include "synonym_parser.h"
#include <boost/lexical_cast.hpp>
#include "utils/csv.h"
#include "utils/configuration.h"
#include <boost/algorithm/string.hpp>


namespace ed{ namespace connectors{

SynonymParser::SynonymParser(const std::string & filename): filename(filename){
        logger = log4cplus::Logger::getInstance("log");
}

void SynonymParser::fill_synonyms(){
    std::string key, value;
    CsvReader reader(this->filename, ',', true, true);
    if(!reader.is_open()) {
       throw SynonymParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"key" , "value"};
    if(!reader.validate(mandatory_headers)) {
        throw SynonymParserException("Impossible to parse file "
                                    + reader.filename + " . Not find column : "
                                    + reader.missing_headers(mandatory_headers));
    }

    int key_c = reader.get_pos_col("key"), value_c = reader.get_pos_col("value");
    while(!reader.eof()) {
        auto row = reader.next();
        if (reader.is_valid(key_c, row) && reader.has_col(value_c, row)){
                key = row[key_c];
                value = row[value_c];
                boost::to_lower(key);
                boost::to_lower(value);
                this->synonym_map[key]=value;
            }
    }
    LOG4CPLUS_TRACE(logger, "synonyms : " + this->synonym_map.size());
}

void SynonymParser::fill(){
    this->fill_synonyms();
}
}}
