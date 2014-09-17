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
#include <boost/program_options.hpp>
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
        ("BROKER.exchange", po::value<std::string>()->default_value("navitia"), "exchange used in rabbitmq");

    return desc;

}

void Configuration::load(const std::string& filename){
    po::options_description desc = get_options_description();
    std::ifstream stream(filename);

    if(!stream.is_open() || !stream.good()){
        throw navitia::exception("impossible to open: " + filename);
    }
    auto tmp = po::parse_config_file(stream, desc, true);
    po::store(tmp, this->vm);
    po::notify(this->vm);

}

}}//namespace
