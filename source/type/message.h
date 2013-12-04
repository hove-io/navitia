#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>


#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

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

enum MessageStatus{
    information = 0,
    warning = 1,
    disrupt = 2
};

struct LocalizedMessage{
    std::string title;
    std::string body;

    template<class Archive> void serialize(Archive & ar, const unsigned int){
        ar & title & body;
    }
};

struct AtPerturbation{
    std::string uri;

    Type_e object_type;
    std::string object_uri;

    boost::posix_time::time_period application_period;

    boost::posix_time::time_duration application_daily_start_hour;
    boost::posix_time::time_duration application_daily_end_hour;

    std::bitset<8> active_days;

    AtPerturbation(): object_type(Type_e::ValidityPattern),
        application_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)){}

    bool valid_day_of_week(const boost::gregorian::date& date) const;

    bool valid_hour_perturbation(const boost::posix_time::time_period& period) const;

    bool is_applicable(const boost::posix_time::time_period& time) const;

    bool operator<(const AtPerturbation& other) const {
        return (this->uri < other.uri);
    }
};

/**
 * les messages étant pour le moment des perturbations AT avec plus d'info
 * on en hérite pour ne pas mutliplier le code
 */
struct Message: public AtPerturbation{
    boost::posix_time::time_period publication_period;
    std::map<std::string, LocalizedMessage> localized_messages;
    MessageStatus message_status;

    Message(): publication_period(boost::posix_time::not_a_date_time,
            boost::posix_time::seconds(0)),
        message_status(MessageStatus::information){}

    template<class Archive> void serialize(Archive & ar, const unsigned int){
        ar & uri & object_type & object_uri & publication_period
            & application_period & application_daily_start_hour
            & application_daily_end_hour & active_days & localized_messages & message_status;
    }

    bool is_valid(const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_time)const;

    bool is_publishable(const boost::posix_time::ptime& time) const;
};

struct MessageHolder{
    // external_code => message
    std::map<std::string, boost::shared_ptr<Message>> messages;


    MessageHolder(){}

    template<class Archive> void serialize(Archive& ar, const unsigned int){
        ar & messages;
    }

    MessageHolder& operator=(const navitia::type::MessageHolder&&);

};

}}//namespace
