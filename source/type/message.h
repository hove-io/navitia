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

enum Jours {
    Lun = 0x01,
    Mar = 0x02,
    Mer = 0x04,
    Jeu = 0x08,
    Ven = 0x10,
    Sam = 0x20,
    Dim = 0x40,
    Fer = 0x80
};

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

    Message(): object_type(Type_e::ValidityPattern),
        publication_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)),
        application_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)){}

    template<class Archive> void serialize(Archive & ar, const unsigned int){
        ar & uri & object_type & object_uri & publication_period
            & application_period & application_daily_start_hour
            & application_daily_end_hour & active_days & message & title;
    }

    bool is_valid(const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_time)const;

    bool valid_day_of_week(const boost::gregorian::date& date) const;

    bool valid_hour_perturbation(const boost::posix_time::time_period& period) const;

    bool is_publishable(const boost::posix_time::ptime& time) const;
    bool is_applicable(const boost::posix_time::time_period& time) const;


    bool operator<(const Message& other) const {
        return (this->uri < other.uri);
    }

};

struct MessageHolder{

    static const unsigned int version = 1; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
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

    std::vector<Message> find_messages(const std::string& uri, const boost::posix_time::ptime& now,
        const boost::posix_time::time_period& action_time) const;

    private:
    void load_lz4(const std::string & filename);

    void save_lz4(const std::string & filename);


};

}}//namespace
BOOST_CLASS_VERSION(navitia::type::MessageHolder, navitia::type::MessageHolder::version)
