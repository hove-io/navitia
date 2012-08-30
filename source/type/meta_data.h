#pragma once
#include "config.h"
#include <string>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

namespace navitia { namespace type {

struct MetaData{

    static const unsigned int data_version = 1; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
    unsigned int version; //< Numéro de version des données chargées

    boost::gregorian::date_period production_date;  
    boost::posix_time::ptime publication_date;


    std::string navimake_version;

    std::vector<std::string> data_sources;


    MetaData() : production_date(boost::gregorian::date(), boost::gregorian::date()), navimake_version(NAVITIA_VERSION) {}

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int version) {
        this->version = version;
        if(this->version != data_version){
            std::cerr << "Attention le fichier de données est à la version " << version << " (version actuelle : " << data_version << ")" << std::endl;
        }

        ar & production_date & publication_date & navimake_version & data_sources;
    }

    friend class boost::serialization::access;

};
}}
BOOST_CLASS_VERSION(navitia::type::MetaData, navitia::type::MetaData::data_version)
