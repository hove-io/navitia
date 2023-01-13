/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "configuration.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>

#include <fstream>
#include <iostream>
#include "utils/functions.h"

namespace po = boost::program_options;

namespace navitia {
namespace kraken {

po::options_description get_options_description(const boost::optional<std::string>& name,
                                                const boost::optional<std::string>& zmq,
                                                const boost::optional<bool>& display_contributors) {
    po::options_description desc("Allowed options");
    // clang-format off
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
        ("GENERAL.is_realtime_add_enabled", po::value<bool>()->default_value(false),
                                        "enable loading of realtime data that add stop_times")
        ("GENERAL.is_realtime_add_trip_enabled", po::value<bool>()->default_value(false),
                                        "enable loading of realtime data that add trips (only if is_realtime_add_enabled is activated)")
        ("GENERAL.kirin_timeout", po::value<int>()->default_value(60000),
                                  "timeout in ms for loading realtime data from kirin")
        ("GENERAL.kirin_retry_timeout", po::value<int>()->default_value(5*60*1000),
                                  "timeout in ms before retrying to load realtime data")
        ("GENERAL.slow_request_duration", po::value<int>()->default_value(1000),
                                  "request running a least this number of milliseconds are logged")

        ("GENERAL.display_contributors", display_contributors ?
             po::value<bool>()->default_value(*display_contributors) : po::value<bool>()->default_value(false),
         "display all contributors in feed publishers")
        ("GENERAL.raptor_cache_size", po::value<int>()->default_value(10), "maximum number of stored raptor caches")
        ("GENERAL.log_level", po::value<std::string>(), "log level of kraken")
        ("GENERAL.log_format", po::value<std::string>()->default_value("[%D{%y-%m-%d %H:%M:%S,%q}] [%p] [%x] - %m %b:%L  %n"), "log format")

        ("GENERAL.enable_request_deadline", po::value<bool>()->default_value(true), "enable deadline of request")
        ("GENERAL.metrics_binding", po::value<std::string>(), "IP:PORT to serving metrics in http")
        ("GENERAL.core_file_size_limit", po::value<int>()->default_value(0), "ulimit that define the maximum size of a core file")

        ("BROKER.uri", po::value<std::string>(), "rabbitmq connection uri")
        ("BROKER.protocol", po::value<std::string>()->default_value("amqp"), "rabbitmq connection protocol")
        ("BROKER.host", po::value<std::string>()->default_value("localhost"), "host of rabbitmq")
        ("BROKER.port", po::value<int>()->default_value(5672), "port of rabbitmq")
        ("BROKER.username", po::value<std::string>()->default_value("guest"), "username for rabbitmq")
        ("BROKER.password", po::value<std::string>()->default_value("guest"), "password for rabbitmq")
        ("BROKER.vhost", po::value<std::string>()->default_value("/"), "vhost for rabbitmq")
        ("BROKER.exchange", po::value<std::string>()->default_value("navitia"), "exchange used in rabbitmq")
        ("BROKER.rt_topics", po::value<std::vector<std::string>>(), "list of realtime topic for this instance")
        ("BROKER.timeout", po::value<int>()->default_value(200), "timeout for maintenance worker in millisecond")
        ("BROKER.max_batch_nb", po::value<int>()->default_value(5000), "max number of realtime messages retrieved in a batch")
        ("BROKER.total_retrieving_timeout", po::value<int>()->default_value(10000), "max total duration the worker is going to spend when retrieving messages")
        ("BROKER.sleeptime", po::value<int>()->default_value(1), "sleeptime for maintenance worker in second")
        ("BROKER.reconnect_wait", po::value<int>()->default_value(1), "Wait duration between connection attempts to rabbitmq, in seconds")
        ("BROKER.queue", po::value<std::string>(), "rabbitmq's queue name to be bound")
        ("BROKER.queue_auto_delete", po::value<bool>()->default_value(false), "auto delete rabbitmq's queue when unbind")
        ("BROKER.queue_expire", po::value<int>()->default_value(7200), "Rabbitmq queues created by kraken will be deleted by rabbitmq after this duration , in seconds")

        ("CHAOS.database", po::value<std::string>(), "Chaos database connection string")
        ("CHAOS.batch_size", po::value<int>()->default_value(1000000), "Chaos database row batch size");

    // clang-format on
    return desc;
}

static std::string env_parser(std::string env) {
    if (!boost::algorithm::starts_with(env, "KRAKEN_")) {
        return "";
    }
    boost::algorithm::replace_first(env, "KRAKEN_", "");
    boost::algorithm::replace_first(env, "_", ".");
    if (!boost::algorithm::starts_with(env, "GENERAL") && !boost::algorithm::starts_with(env, "BROKER")
        && !boost::algorithm::starts_with(env, "CHAOS")) {
        // it doesn't look like one of our var, we ignore it
        // docker and kubernetes define a lot of env var starting by kraken when deploying kraken
        // and boost po will abort startup if unknown var are present
        return "";
    }
    return env;
}

void Configuration::load(const std::string& filename) {
    po::options_description desc = get_options_description();

    po::store(po::parse_environment(desc, env_parser), this->vm);

    std::ifstream stream(filename);
    if (!stream.is_open() || !stream.good()) {
        std::cerr << "no configuration file found, using only environment variables" << std::endl;
    } else {
        // we allow unknown option for log4cplus
        po::store(po::parse_config_file(stream, desc, true), this->vm);
    }
    po::notify(this->vm);
}

std::vector<std::string> Configuration::load_from_command_line(const po::options_description& desc,
                                                               int argc,
                                                               const char* const argv[]) {
    auto tmp = po::basic_command_line_parser<char>(argc, argv).options(desc).allow_unregistered().run();
    po::store(tmp, vm);
    po::notify(vm);

    // return unparsed options
    return po::collect_unrecognized(tmp.options, po::include_positional);
}

std::string Configuration::databases_path() const {
    return this->vm["GENERAL.database"].as<std::string>();
}
std::string Configuration::zmq_socket_path() const {
    return this->vm["GENERAL.zmq_socket"].as<std::string>();
}
std::string Configuration::instance_name() const {
    return this->vm["GENERAL.instance_name"].as<std::string>();
}
boost::optional<std::string> Configuration::chaos_database() const {
    boost::optional<std::string> result;
    if (this->vm.count("CHAOS.database") > 0) {
        result = this->vm["CHAOS.database"].as<std::string>();
    }
    return result;
}
int Configuration::chaos_batch_size() const {
    int batch_size = vm["CHAOS.batch_size"].as<int>();
    if (batch_size < 0) {
        throw std::invalid_argument("chaos.batch_size cannot be negative");
    }
    return batch_size;
}

int Configuration::nb_threads() const {
    int nb_threads = vm["GENERAL.nb_threads"].as<int>();
    if (nb_threads < 0) {
        throw std::invalid_argument("nb_threads cannot be negative");
    }
    return size_t(nb_threads);
}

bool Configuration::is_realtime_enabled() const {
    return this->vm["GENERAL.is_realtime_enabled"].as<bool>();
}

bool Configuration::is_realtime_add_enabled() const {
    return this->vm["GENERAL.is_realtime_add_enabled"].as<bool>();
}

bool Configuration::is_realtime_add_trip_enabled() const {
    return this->vm["GENERAL.is_realtime_add_trip_enabled"].as<bool>();
}

int Configuration::kirin_timeout() const {
    return this->vm["GENERAL.kirin_timeout"].as<int>();
}

int Configuration::core_file_size_limit() const {
    return this->vm["GENERAL.core_file_size_limit"].as<int>();
}

boost::optional<std::string> Configuration::broker_uri() const {
    if (this->vm.count("BROKER.uri") > 0) {
        return this->vm["BROKER.uri"].as<std::string>();
    }
    return {};
}
std::string Configuration::broker_protocol() const {
    return this->vm["BROKER.protocol"].as<std::string>();
}
std::string Configuration::broker_host() const {
    return this->vm["BROKER.host"].as<std::string>();
}
int Configuration::broker_port() const {
    return this->vm["BROKER.port"].as<int>();
}
std::string Configuration::broker_username() const {
    return this->vm["BROKER.username"].as<std::string>();
}
std::string Configuration::broker_password() const {
    return this->vm["BROKER.password"].as<std::string>();
}
std::string Configuration::broker_vhost() const {
    return this->vm["BROKER.vhost"].as<std::string>();
}
std::string Configuration::broker_exchange() const {
    return this->vm["BROKER.exchange"].as<std::string>();
}

int Configuration::broker_timeout() const {
    return vm["BROKER.timeout"].as<int>();
}

int Configuration::total_retrieving_timeout() const {
    return vm["BROKER.total_retrieving_timeout"].as<int>();
}

int Configuration::broker_max_batch_nb() const {
    return vm["BROKER.max_batch_nb"].as<int>();
}

int Configuration::broker_sleeptime() const {
    return vm["BROKER.sleeptime"].as<int>();
}

int Configuration::broker_reconnect_wait() const {
    return vm["BROKER.reconnect_wait"].as<int>();
}

std::string Configuration::broker_queue(const std::string& default_queue) const {
    if (vm.count("BROKER.queue")) {
        return this->vm["BROKER.queue"].as<std::string>();
    }
    return default_queue;
}

bool Configuration::broker_queue_auto_delete() const {
    return vm["BROKER.queue_auto_delete"].as<bool>();
}

int Configuration::broker_queue_expire() const {
    return vm["BROKER.queue_expire"].as<int>();
}

std::vector<std::string> Configuration::rt_topics() const {
    if (!this->vm.count("BROKER.rt_topics")) {
        return std::vector<std::string>();
    }
    std::vector<std::string> result = this->vm["BROKER.rt_topics"].as<std::vector<std::string>>();
    if (result.size() == 1) {
        return split_string(result.at(0), ";");
    }
    return result;
}

int Configuration::kirin_retry_timeout() const {
    return vm["GENERAL.kirin_retry_timeout"].as<int>();
}

bool Configuration::display_contributors() const {
    if (!this->vm.count("GENERAL.display_contributors")) {
        return false;
    }
    return vm["GENERAL.display_contributors"].as<bool>();
}

int Configuration::slow_request_duration() const {
    return vm["GENERAL.slow_request_duration"].as<int>();
}

bool Configuration::enable_request_deadline() const {
    return vm["GENERAL.enable_request_deadline"].as<bool>();
}

size_t Configuration::raptor_cache_size() const {
    if (!vm.count("GENERAL.raptor_cache_size")) {
        return 10;
    }
    int raptor_cache_size = vm["GENERAL.raptor_cache_size"].as<int>();
    if (raptor_cache_size < 1) {
        throw std::invalid_argument("raptor_cache_size must be strictly positive");
    }
    return size_t(raptor_cache_size);
}

boost::optional<std::string> Configuration::log_level() const {
    boost::optional<std::string> result;
    if (this->vm.count("GENERAL.log_level") > 0) {
        result = this->vm["GENERAL.log_level"].as<std::string>();
    }
    return result;
}
boost::optional<std::string> Configuration::log_format() const {
    boost::optional<std::string> result;
    if (this->vm.count("GENERAL.log_format") > 0) {
        result = this->vm["GENERAL.log_format"].as<std::string>();
    }
    return result;
}

boost::optional<std::string> Configuration::metrics_binding() const {
    boost::optional<std::string> result;
    if (this->vm.count("GENERAL.metrics_binding") > 0) {
        result = this->vm["GENERAL.metrics_binding"].as<std::string>();
    }
    return result;
}
}  // namespace kraken
}  // namespace navitia
