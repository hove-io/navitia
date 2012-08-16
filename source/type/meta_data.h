#pragma once
#include <string>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace navitia { namespace type {

struct MetaData{
    boost::gregorian::date_period production_date;  
    boost::posix_time::ptime publication_date;


    std::vector<std::string> data_sources;


    MetaData() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}

};

}}
