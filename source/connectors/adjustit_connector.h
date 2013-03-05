#pragma once

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include "type/message.h"
#include <boost/program_options.hpp>

namespace navitia{


navitia::type::Type_e parse_object_type(std::string object_type);

struct AtLoader {

    private:

        QSqlDatabase connect(const boost::program_options::variables_map& params);

        QSqlQuery exec(const boost::program_options::variables_map& params, const boost::posix_time::ptime& now);

        navitia::type::Message parse_message(const QSqlQuery& requester);

    public:
        std::map<std::string, std::vector<navitia::type::Message>> load(const boost::program_options::variables_map& params, const boost::posix_time::ptime& now);

};


}//namespace
