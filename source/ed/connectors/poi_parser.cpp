#include "poi_parser.h"
#include "type/data.h"
#include "utils/csv.h"
#include "utils/functions.h"


namespace ed{ namespace connectors{
PoiParser::PoiParser(const std::string & path, const ed::connectors::ConvCoord& conv_coord): path(path), conv_coord(conv_coord){
        logger = log4cplus::Logger::getInstance("log");
}


void PoiParser::fill_poi_type(){
    CsvReader reader(this->path + "/poi_type.txt" , ';', true, true);
    if(!reader.is_open()) {
        throw PoiParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"poi_type_id" , "poi_type_name"};
    if(!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_type_id");
    int name_c = reader.get_pos_col("poi_type_name");
    while(!reader.eof()){
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(name_c, row)){
            const auto& itm = this->data.poi_types.find(row[id_c]);
            if(itm == this->data.poi_types.end()){
                ed::types::PoiType* poi_type = new ed::types::PoiType;
                poi_type->id = this->data.poi_types.size() + 1;
                poi_type->name = row[name_c];
                this->data.poi_types[row[id_c]] = poi_type;
            }
        }
    }
}

void PoiParser::fill_poi(){
    CsvReader reader(this->path + "/poi.txt", ';', true, true);
    if(!reader.is_open()) {
        throw PoiParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"poi_id", "poi_name", "poi_weight", "poi_visible", "poi_lat", "poi_lon", "poi_type_id"};
    if(!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_id");
    int name_c = reader.get_pos_col("poi_name");
    int weight_c = reader.get_pos_col("poi_weight");
    int visible_c = reader.get_pos_col("poi_visible");
    int lat_c = reader.get_pos_col("poi_lat");
    int lon_c = reader.get_pos_col("poi_lon");
    int type_id_c = reader.get_pos_col("poi_type_id");

    while(!reader.eof()){
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(name_c, row)
            && reader.is_valid(weight_c, row) && reader.is_valid(visible_c, row)
            && reader.is_valid(lat_c, row) && reader.is_valid(lon_c, row)
            && reader.is_valid(type_id_c, row)){
            const auto& itm = this->data.pois.find(row[id_c]);
            if(itm == this->data.pois.end()){
                const auto& poi_type = this->data.poi_types.find(row[type_id_c]);
                if(poi_type != this->data.poi_types.end()){
                    ed::types::Poi* poi = new ed::types::Poi;
                    poi->id = this->data.pois.size() + 1;
                    poi->name = row[name_c];
                    try{
                        poi->visible = boost::lexical_cast<bool>(row[visible_c]);
                    }catch(boost::bad_lexical_cast ) {
                        LOG4CPLUS_WARN(logger, "Impossible to parse the visible for " + row[id_c] + " " + row[name_c]);
                        poi->visible = true;
                    }
                    try{
                        poi->weight = boost::lexical_cast<int>(row[weight_c]);
                    }catch(boost::bad_lexical_cast ) {
                        LOG4CPLUS_WARN(logger, "Impossible to parse the weight for " + row[id_c] + " " + row[name_c]);
                        poi->weight = 0;
                    }
                    poi->poi_type = poi_type->second;
                    poi->coord = this->conv_coord.convert_to(navitia::type::GeographicalCoord(str_to_double(row[lon_c]), str_to_double(row[lat_c])));
                    this->data.pois[row[id_c]] = poi;
                }
            }
        }
    }
}

void PoiParser::fill_poi_properties(){
    CsvReader reader(this->path + "/poi_properties.txt", ';', true, true);
    if(!reader.is_open()) {
        LOG4CPLUS_WARN(logger, "Cannot found file : " + reader.filename);
        return;
    }
    std::vector<std::string> mandatory_headers = {"poi_id", "key", "value"};
    if(!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_id");
    int key_c = reader.get_pos_col("key");
    int value_c = reader.get_pos_col("value");

    while(!reader.eof()){
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(key_c, row)
            && reader.is_valid(value_c, row)){
            const auto& itm = this->data.pois.find(row[id_c]);
            if(itm != this->data.pois.end()){
                itm->second->properties[row[key_c]] = row[value_c];
            }else{
                LOG4CPLUS_WARN(logger, "Poi "<<row[id_c]<<" not found");
            }
        }
    }
}

void PoiParser::fill(){
    fill_poi_type();
    fill_poi();
    fill_poi_properties();
}

}}
