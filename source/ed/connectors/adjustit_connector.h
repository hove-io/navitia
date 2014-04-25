/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include "type/message.h"
#include <boost/program_options.hpp>

namespace ed{ namespace connectors{


navitia::type::Type_e parse_object_type(std::string object_type);

std::string get_string_field(const std::string& field, const QSqlQuery& requester, const std::map<std::string, int>& mapping,
        const std::string& default_value="");

int get_int_field(const std::string& field, const QSqlQuery& requester, const std::map<std::string, int>& mapping,
        int default_value=0);

struct AtLoader {

    struct Config{
        std::string connect_string;
        std::string media_lang;
        std::string media_media;
    };

    private:

        QSqlDatabase connect(const Config& params);

        QSqlQuery find_all(const Config& params, const boost::posix_time::ptime& now);
        QSqlQuery find_disrupt(const Config& params, const boost::posix_time::ptime& now);

        navitia::type::Message parse_message(const QSqlQuery& requester, const std::map<std::string, int>& mapping);
        std::map<std::string, int> get_mapping(const QSqlRecord& record);

    public:
        std::map<std::string, std::vector<navitia::type::Message>> load(const Config& params, const boost::posix_time::ptime& now);
        std::map<std::string, std::vector<navitia::type::Message>> load_disrupt(const Config& params, const boost::posix_time::ptime& now);

};


}}//namespace
