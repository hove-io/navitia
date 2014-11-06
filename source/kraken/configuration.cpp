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

namespace po = boost::program_options;

namespace navitia { namespace kraken{

po::options_description get_options_description(){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("GENERAL.database", po::value<std::string>()->default_value("data.nav.lz4"), "path to the data file")
        ("GENERAL.zmq_socket", po::value<std::string>()->required(), "path to the zmq socket used for receiving resquest")
        ("GENERAL.instance_name", po::value<std::string>()->required(), "name of the instance")
        ("GENERAL.nb_threads", po::value<int>()->default_value(1), "number of workers threads")

        ("BROKER.host", po::value<std::string>()->default_value("localhost"), "host of rabbitmq")
        ("BROKER.port", po::value<int>()->default_value(5672), "port of rabbitmq")
        ("BROKER.username", po::value<std::string>()->default_value("guest"), "username for rabbitmq")
        ("BROKER.password", po::value<std::string>()->default_value("guest"), "password for rabbitmq")
        ("BROKER.vhost", po::value<std::string>()->default_value("/"), "vhost for rabbitmq")
        ("BROKER.exchange", po::value<std::string>()->default_value("navitia"), "exchange used in rabbitmq")
        ("BROKER.rt_topics", po::value<std::vector<std::string>>(), "list of realtime topic for this instance")

        ("CHAOS.activation", po::value<bool>()->default_value(false), "Activate or desactivate connection to chaos database")
        ("CHAOS.database", po::value<std::string>()->default_value(""), "Chaos database connection string");

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

void Configuration::load_from_command_line(int argc, const char* const argv[]) {
    po::options_description desc = get_options_description();
    auto tmp = po::parse_command_line(argc, argv, desc);
    po::store(tmp, vm);
    po::notify(vm);
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
bool Configuration::is_chaos_active() const{
    return this->vm["CHAOS.activation"].as<bool>();
}
std::string Configuration::chaos_database() const{
    return this->vm["CHAOS.database"].as<std::string>();
}
int Configuration::nb_thread() const{
    return this->vm["GENERAL.nb_threads"].as<int>();
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

std::vector<std::string> Configuration::rt_topics() const{
    if(! this->vm.count("BROKER.rt_topics")){
        return std::vector<std::string>();
    }
    return this->vm["BROKER.rt_topics"].as<std::vector<std::string>>();
}
}}//namespace
