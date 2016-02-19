/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "configuration.h"
#include "utils/exception.h"
#include <fstream>
#include <boost/optional.hpp>

namespace po = boost::program_options;

namespace navitia { namespace kraken{

po::options_description get_options_description(const boost::optional<std::string> name,
                                                const boost::optional<std::string> zmq,
                                                const boost::optional<bool> display_contributors ){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("GENERAL.database", po::value<std::string>()->default_value("data.nav.lz4"), "path to the data file")

            //name and zmq socket can have default values (for tests)
        ("GENERAL.zmq_socket",
         zmq ? po::value<std::string>()->default_value(*zmq) : po::value<std::string>()->required(),
         "path to the zmq socket used for receiving resquest")
        ("GENERAL.instance_name",
         name ? po::value<std::string>()->default_value(*name) : po::value<std::string>()->required(),
         "name of the instance")

        ("GENERAL.nb_threads", po::value<int>()->default_value(1), "number of workers threads")
        ("GENERAL.is_realtime_enabled", po::value<bool>()->default_value(false),
                                        "enable loading of realtime data")
        ("GENERAL.kirin_timeout", po::value<int>()->default_value(60000),
                                  "timeout in ms for loading realtime data from kirin")
        ("GENERAL.kirin_retry_timeout", po::value<int>()->default_value(5*60*1000),
                                  "timeout in ms before retrying to load realtime data")

        ("GENERAL.display_contributors", display_contributors ?
             po::value<bool>()->default_value(*display_contributors) : po::value<bool>()->default_value(false),
         "display all contributors in feed publishers")
        ("GENERAL.raptor_cache_size", po::value<int>()->default_value(10), "maximum number of stored raptor caches")

        ("BROKER.host", po::value<std::string>()->default_value("localhost"), "host of rabbitmq")
        ("BROKER.port", po::value<int>()->default_value(5672), "port of rabbitmq")
        ("BROKER.username", po::value<std::string>()->default_value("guest"), "username for rabbitmq")
        ("BROKER.password", po::value<std::string>()->default_value("guest"), "password for rabbitmq")
        ("BROKER.vhost", po::value<std::string>()->default_value("/"), "vhost for rabbitmq")
        ("BROKER.exchange", po::value<std::string>()->default_value("navitia"), "exchange used in rabbitmq")
        ("BROKER.rt_topics", po::value<std::vector<std::string>>(), "list of realtime topic for this instance")
        ("BROKER.timeout", po::value<int>()->default_value(100), "timeout for maintenance worker in millisecond")
        ("BROKER.sleeptime", po::value<int>()->default_value(1), "sleeptime for maintenance worker in second")

        ("CHAOS.database", po::value<std::string>(), "Chaos database connection string");

    return desc;

}

void Configuration::load(const std::string& filename){
    po::options_description desc = get_options_description();
    std::ifstream stream(filename);

    if(!stream.is_open() || !stream.good()){
        throw navitia::exception("impossible to open: " + filename);
    }
    //we allow unknown option for log4cplus
    auto tmp = po::parse_config_file(stream, desc, true);
    po::store(tmp, this->vm);
    po::notify(this->vm);
}

std::vector<std::string> Configuration::load_from_command_line(const po::options_description& desc, int argc, const char* const argv[]) {
    auto tmp = po::basic_command_line_parser<char>(argc, argv).options(desc).allow_unregistered().run();
    po::store(tmp, vm);
    po::notify(vm);

    //return unparsed options
    return po::collect_unrecognized(tmp.options, po::include_positional);
}

std::string Configuration::databases_path() const{
    return this->vm["GENERAL.database"].as<std::string>();
}
std::string Configuration::zmq_socket_path() const{
    return this->vm["GENERAL.zmq_socket"].as<std::string>();
}
std::string Configuration::instance_name() const{
    return this->vm["GENERAL.instance_name"].as<std::string>();
}
boost::optional<std::string> Configuration::chaos_database() const{
    boost::optional<std::string> result;
    if (this->vm.count("CHAOS.database") > 0) {
        result = this->vm["CHAOS.database"].as<std::string>();
    }
    return result;
}
int Configuration::nb_threads() const{
    int nb_threads = vm["GENERAL.nb_threads"].as<int>();
    if (nb_threads < 0) {
        throw std::invalid_argument("nb_threads cannot be negative");
    }
    return size_t(nb_threads);
}

bool Configuration::is_realtime_enabled() const{
    return this->vm["GENERAL.is_realtime_enabled"].as<bool>();
}

int Configuration::kirin_timeout() const{
    return this->vm["GENERAL.kirin_timeout"].as<int>();
}

std::string Configuration::broker_host() const{
    return this->vm["BROKER.host"].as<std::string>();
}
int Configuration::broker_port() const{
    return this->vm["BROKER.port"].as<int>();
}
std::string Configuration::broker_username() const{
    return this->vm["BROKER.username"].as<std::string>();
}
std::string Configuration::broker_password() const{
    return this->vm["BROKER.password"].as<std::string>();
}
std::string Configuration::broker_vhost() const{
    return this->vm["BROKER.vhost"].as<std::string>();
}
std::string Configuration::broker_exchange() const{
    return this->vm["BROKER.exchange"].as<std::string>();
}

int Configuration::broker_timeout() const {
    return vm["BROKER.timeout"].as<int>();
}

int Configuration::broker_sleeptime() const {
    return vm["BROKER.sleeptime"].as<int>();
}

std::vector<std::string> Configuration::rt_topics() const{
    if(! this->vm.count("BROKER.rt_topics")){
        return std::vector<std::string>();
    }
    return this->vm["BROKER.rt_topics"].as<std::vector<std::string>>();
}

int Configuration::kirin_retry_timeout() const{
    return vm["GENERAL.kirin_retry_timeout"].as<int>();
}

bool Configuration::display_contributors() const{
    if(!this->vm.count("GENERAL.display_contributors")){
        return false;
    }
    return vm["GENERAL.display_contributors"].as<bool>();
}

size_t Configuration::raptor_cache_size() const{
    if (! vm.count("GENERAL.raptor_cache_size")) {
        return 10;
    }
    int raptor_cache_size = vm["GENERAL.raptor_cache_size"].as<int>();
    if (raptor_cache_size < 0) {
        throw std::invalid_argument("raptor_cache_size cannot be negative");
    }
    return size_t(raptor_cache_size);
}
}}//namespace
