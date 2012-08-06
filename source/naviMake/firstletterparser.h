#pragma once

#include <string>
#include "data.h"

namespace navimake{ namespace connectors{

using navitia::type::idx_t;

class FirstletterParser
{
public:
    FirstletterParser(const std::string & path) : path(path) {}

    /**
      * Charge tout
      *
      */
    void load(navimake::Data & data);

    /**
     * Charge les Cities dans un objet Data de Navimake
     *
     */
    void load_city(navimake::Data& data);

    /**
     * Charge les departmens dans un objet Data de Navimake
     *
     */
    void load_department(navimake::Data& data);

    /**
     * Charge les Districts dans un objet Data de Navimake
     *
     */
    void load_district(navimake::Data& data);
    /**
     * Charge les Addresses dans un objet Data de Navimake
     *
     */
    void load_address(navitia::type::Data &nav_data);

private:
    std::string path;

    std::map<std::string, navimake::types::District*> district_map;
    std::map<std::string, navimake::types::Department*> department_map;
    std::map<std::string, navimake::types::City*> city_map;
};

}}
