#pragma once
#include "boost/date_time/posix_time/posix_time.hpp"
#include "type/type.h"
#include "type/pt_data.h"

namespace navitia { namespace disruption {


struct disrupt{
    type::idx_t idx;
    type::idx_t network_idx;
    std::vector<type::idx_t> line_idx;
    std::vector<type::idx_t> stop_area_idx;
};

class Disruption{
private:

    std::vector<disrupt> disrupts;
    log4cplus::Logger logger;

    type::idx_t find_or_create(const type::Network* network);
    void add_stop_areas(const std::vector<type::idx_t>& network_idx,
                      const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());

    void add_networks(const std::vector<type::idx_t>& network_idx,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
    void add_lines(const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
public:
    Disruption():logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"))){};

    void disruptions_list(const std::string& filter,
                    const std::vector<std::string>& forbidden_uris,
                    const type::Data &d,
                    const boost::posix_time::time_period action_period,
                    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());

    const std::vector<disrupt>& get_disrupts() const;
    size_t get_disrupts_size();
};
}}

