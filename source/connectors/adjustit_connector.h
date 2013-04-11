#pragma once

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include "type/message.h"
#include <boost/program_options.hpp>

namespace navitia{


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


}//namespace
