#pragma once

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include "type/message.h"
#include <boost/program_options.hpp>

namespace navitia{


navitia::type::Type_e parse_object_type(std::string object_type);

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

        navitia::type::Message parse_message(const QSqlQuery& requester);

    public:
        std::map<std::string, std::vector<navitia::type::Message>> load(const Config& params, const boost::posix_time::ptime& now);
        std::map<std::string, std::vector<navitia::type::Message>> load_disrupt(const Config& params, const boost::posix_time::ptime& now);

};


}//namespace
