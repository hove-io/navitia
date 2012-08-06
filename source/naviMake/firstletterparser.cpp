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
    CsvReader reader(path + "/T_DEPARTMENT.csv", ';', "utf16");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["[DEP_NAME]"];
    size_t code = cols["[DEP_CODE]"];
    size_t rcode = cols["[REG_CODE]"];

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
    CsvReader reader(path + "/T_REGION.csv", ';', "utf16");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["[REG_NAME]"];
    size_t code = cols["[REG_CODE]"];

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
    CsvReader reader(path + "/T_CITY.csv", ';', "utf16");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["[CIT_NAME]"];
    size_t code = cols["[CIT_INSEE]"];
    size_t dcode = cols["[DEP_CODE]"];
    size_t cit_x = cols["[CIT_X]"];
    size_t cit_y = cols["[CIT_Y]"];
    size_t cit_cp = cols["[CIT_CP]"];

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 6)
            continue;
        navimake::types::City* city = new navimake::types::City();
        city->external_code= row[code];
        city->name = row[name];
        city->department = department_map.at(row[dcode]);
        city->coord.x = boost::lexical_cast<double>(row[cit_x]);
        city->coord.y = boost::lexical_cast<double>(row[cit_y]);
        city->main_postal_code = row[cit_cp];

        data.cities.push_back(city);
        city_map[city->external_code] = city;
    }
}

void FirstletterParser::load_address(navitia::type::Data &nav_data) {
    std::cout << "On parse les adresses " << std::endl;
    CsvReader reader(path + "/T_ADDRESS.csv", ';', "utf16");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["[ADD_NAME]"];
    size_t city = cols["[CIT_INSEE]"];

    std::unordered_map<std::string, bool> ways;

    int idx = 0;
    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        if(ways.find(row[name]+row[city]) == ways.end()) {
            navitia::streetnetwork::Way way;
            way.name = row[name];
            way.city_idx = nav_data.pt_data.city_map.at(row[city]);
            way.idx = idx;
            nav_data.street_network.ways.push_back(way);
            ways[row[name]+row[city]] = true;
            ++idx;

        }
    }
}
}}
