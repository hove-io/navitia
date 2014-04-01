#pragma once
#include "config.h"
#include <string>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

namespace navitia { namespace type {

struct MetaData{

    boost::gregorian::date_period production_date;
    boost::posix_time::ptime publication_date;


    std::string navimake_version;

    std::vector<std::string> data_sources;

    std::string shape;

    MetaData() : production_date(boost::gregorian::date(), boost::gregorian::date()),
    navimake_version(KRAKEN_VERSION) {}

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & production_date & publication_date & navimake_version & data_sources & shape;
    }

    friend class boost::serialization::access;

};
}}
