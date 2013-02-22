#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <atomic>
#include <map>
#include <vector>
#include <string>

#include "type/type.h"


namespace navitia { namespace type {

struct Message{
    std::string uri;

    Type_e object_type;
    std::string object_uri;

    boost::posix_time::time_period publication_period;

    boost::posix_time::time_period application_period;

    boost::posix_time::time_duration application_daily_start_hour;
    boost::posix_time::time_duration application_daily_end_hour;

    std::bitset<7> active_days;

    std::string message;
    std::string title;

    Message(): object_type(Type_e::eValidityPattern), 
        publication_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)),
        application_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)){}

    template<class Archive> void serialize(Archive & ar, const unsigned int){
        ar & uri & object_type & object_uri & publication_period
            & application_period & application_daily_start_hour
            & application_daily_end_hour & active_days & message & title;
    }

};

struct MessageHolder{

    //UTC
    boost::posix_time::ptime generation_date;

    //UTC
    boost::posix_time::ptime last_load_at;

    // object_external_code => vector<message>
    std::map<std::string, std::vector<Message>> messages;


    std::atomic<bool> last_load;

    std::atomic<bool> loaded;

    MessageHolder(): last_load(false), loaded(false){}

    template<class Archive> void serialize(Archive & ar, const unsigned int){
        ar & messages & generation_date;
    }

    /** Charge les données et effectue les initialisations nécessaires */
    bool load(const std::string & filename);

    /** Sauvegarde les données */
    void save(const std::string & filename);

    MessageHolder& operator=(const navitia::type::MessageHolder&&);

    private:
    void load_lz4(const std::string & filename);

    void save_lz4(const std::string & filename);


};

}}//namespace
