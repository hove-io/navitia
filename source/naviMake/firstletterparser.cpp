#include "firstletterparser.h"
#include "utils/csv.h"
#include "boost/lexical_cast.hpp"
namespace navimake { namespace  connectors {

void FirstletterParser::load(Data &data) {
    load_district(data);
    load_department(data);
    load_city(data);
}


void FirstletterParser::load_department(Data & data) {
    CsvReader reader(path + "/departments.csv");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["name"];
    size_t code = cols["order07"];
    size_t rcode = cols["order01"];

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 3)
            continue;
        navimake::types::Department* department = new navimake::types::Department();
        department->external_code= row[code];
        department->name = row[name];
        department->district = district_map[row[rcode]];
        data.departments.push_back(department);
        department_map[department->external_code] = department;
    }
}

void FirstletterParser::load_district(Data & data) {
    CsvReader reader(path + "/regions.csv");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["name"];
    size_t code = cols["order01"];

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        navimake::types::District* district= new navimake::types::District();
        district->external_code= row[code];
        district->name = row[name];
        data.districts.push_back(district);
        district_map[district->external_code] = district;
    }
}

void FirstletterParser::load_city(Data & data) {
    CsvReader reader(path + "/cities.csv");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["nom"];
    size_t code = cols["code_insee"];
    size_t dcode = cols["departement"];
    size_t cit_x = cols["x"];
    size_t cit_y = cols["y"];
    //size_t cit_cp = cols["postcode"];

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 6)
            continue;
        navimake::types::City* city = new navimake::types::City();
        city->external_code= row[code];
        city->name = row[name];
        city->department = department_map.at(row[dcode]);
        city->coord.x = boost::lexical_cast<double>(row[cit_x]);
        city->coord.y = boost::lexical_cast<double>(row[cit_y]);
        //city->main_postal_code = row[cit_cp];

        data.cities.push_back(city);
        city_map[city->external_code] = city;
    }
}

void FirstletterParser::load_address(navitia::type::Data &nav_data) {
    std::cout << "On parse les adresses " << std::endl;
    CsvReader reader(path + "/adress_france.csv");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["name"];
    size_t left_city = cols["insee_gauche"];
    size_t rigth_city = cols["insee_droite"];

    std::unordered_map<std::string, bool> ways;



    int idx = 0;
    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;

        if(ways.find(row[name]+row[left_city]) == ways.end()) {
            //navitia::streetnetwork::Way way;
            navitia::georef::Way way;
            way.name = row[name];
            if(nav_data.pt_data.city_map.find(row[left_city]) != nav_data.pt_data.city_map.end()){
                way.city_idx = nav_data.pt_data.city_map.at(row[left_city]);
            }else if(nav_data.pt_data.city_map.find(row[rigth_city]) != nav_data.pt_data.city_map.end()){
                way.city_idx = nav_data.pt_data.city_map.at(row[rigth_city]);
            }
            way.idx = idx;
            //nav_data.street_network.ways.push_back(way);
            nav_data.geo_ref.ways.push_back(way);
            ways[row[name]+row[left_city]] = true;
            ++idx;

        }
    }
}
}}
