#include "connectors.h"

#include <boost/lexical_cast.hpp>
#include "csv.h"

using namespace BO::connectors;

CsvFusio::CsvFusio(const std::string& path): path(path){}

void CsvFusio::fill(BO::Data& data){
    fill_networks(data);
    fill_modes_type(data);
    fill_modes(data);
    fill_lines(data);
}

void CsvFusio::fill_networks(BO::Data& data){
    
    CsvReader reader(path + "Network.csv");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 11){
            throw BadFormated();
        }
        if(counter != 0){
            Network* network = new Network();
            network->id = boost::lexical_cast<int>(row[0]);
            network->name = row[2];
            network->external_code = row[3];
            network->address_name = row[5];
            network->address_number = row[6];
            network->address_type_name = row[7];
            network->phone_number = row[7];
            network->mail = row[8];
            network->website = row[9];
            network->fax = row[1];
            //@TODO gérer les autres champs
            data.networks.push_back(network);
            network_map[network->id] = network; 
        }
        counter++;
    }
    std::cout << data.networks.size() << std::endl;
    reader.close();
}


void CsvFusio::fill_modes_type(BO::Data& data){
    
    CsvReader reader(path + "ModeType.csv");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 3){
            throw BadFormated();
        }
        if(counter != 0){
            ModeType* mode_type = new ModeType();
            mode_type->id = boost::lexical_cast<int>(row[0]);
            mode_type->name = row[1];
            mode_type->external_code = row[2];
            //@TODO gérer les autres champs
            data.mode_types.push_back(mode_type);
            mode_type_map[mode_type->id] = mode_type; 
        }
        counter++;       
    }
    std::cout << data.mode_types.size() << std::endl;
    reader.close();
}

void CsvFusio::fill_modes(BO::Data& data){
    
    CsvReader reader(path + "Mode.csv");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 5){
            throw BadFormated();
        }
        if(counter != 0){
            Mode* mode = new Mode();
            mode->id = boost::lexical_cast<int>(row[0]);
            mode->name = row[2];
            mode->external_code = row[3];

            int mode_type_id = boost::lexical_cast<int>(row[1]);
            mode->mode_type = this->find(mode_type_map, mode_type_id);

            data.modes.push_back(mode);
            mode_map[mode->id] = mode; 
        }
        counter++;       
    }
    std::cout << data.modes.size() << std::endl;
    reader.close();
}


void CsvFusio::fill_lines(BO::Data& data){
    
    CsvReader reader(path + "Line.csv");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 13){
            throw BadFormated();
        }
        if(counter != 0){
            Line* line = new Line();
            line->id = boost::lexical_cast<int>(row[LineMapping::id]);
            line->name = row[LineMapping::name];
            line->external_code = row[LineMapping::external_code];
            line->code = row[LineMapping::code];

            int network_id = boost::lexical_cast<int>(row[LineMapping::network_id]);
            line->network = this->find(network_map, network_id);

            int mode_type_id = boost::lexical_cast<int>(row[1]);
            line->mode_type = this->find(mode_type_map, mode_type_id);

            data.lines.push_back(line);
        }
        counter++;       
    }
    std::cout << data.lines.size() << std::endl;
    reader.close();
}
