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
#include "kraken_zmq.h"

#include "conf.h"
#include "type/type.pb.h"
#include "utils/functions.h"  //navitia::absolute_path function
#include "utils/init.h"
#include "utils/zmq.h"
#include "utils/get_hostname.h"

#include <boost/thread.hpp>
#include <google/protobuf/descriptor.h>

#include <functional>
#include <iostream>
#include <string>
#include <sys/resource.h>  // Posix dependencies for getrlimit

namespace {
void show_usage(const std::string& name, const boost::program_options::options_description& descr) {
    std::cerr << "Usage:\n"
              << "\t" << name << " --help\t\tShow this message.\n"
              << "\t" << name << " [config_file]\tSpecify the path of the configuration file "
              << "(default: kraken.ini in the current path) \n\n"
              << "Configuration file (kraken.ini) can have the following parameter: \n"
              << descr << "\n"
              << "Parameters from the configuration file can also be declared by environment variable like: \n"
              << "\t KRAKEN_GENERAL_instance_name=<inst_name>" << std::endl;
}

void set_core_file_size_limit(int max_core_file_size, log4cplus::Logger& logger) {
    if (max_core_file_size != 0) {
        rlimit limit;

        if (getrlimit(RLIMIT_CORE, &limit) == -1) {
            LOG4CPLUS_ERROR(logger, "Fail to call system 'getrlimit()'");
            return;
        }

        limit.rlim_cur = max_core_file_size;  // Soft Limit

        if (setrlimit(RLIMIT_CORE, &limit) == -1) {
            LOG4CPLUS_ERROR(logger, "Fail to call system 'setrlimit()'");
            return;
        }

        LOG4CPLUS_INFO(logger, "Setting max core file size to : " << max_core_file_size << " bytes");
    }
}
}  // namespace

int main(int argn, char** argv) {
    navitia::init_app();
    navitia::kraken::Configuration conf;

    std::string::size_type posSlash = std::string(argv[0]).find_last_of("\\/");
    std::string application = std::string(argv[0]).substr(posSlash + 1);
    std::string path = navitia::absolute_path();
    if (path.empty()) {
        return 1;
    }
    std::string conf_file;
    if (argn > 1) {
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help") {
            auto opt_desc = navitia::kraken::get_options_description();
            show_usage(argv[0], opt_desc);
            return 0;
        }
        // The first argument is the path to the configuration file
        conf_file = arg;

    } else {
        conf_file = path + application + ".ini";
    }
    conf.load(conf_file);
    auto log_level = conf.log_level();
    auto log_format = conf.log_format();
    if (log_level && log_format) {
        navitia::init_logger("kraken", *log_level, false, *log_format);
    } else {
        navitia::init_logger(conf_file);
    }

    DataManager<navitia::type::Data> data_manager;

    auto logger = log4cplus::Logger::getInstance("startup");
    LOG4CPLUS_INFO(logger, "starting kraken: " << navitia::config::project_version);

    set_core_file_size_limit(conf.core_file_size_limit(), logger);

    boost::thread_group threads;
    // Prepare our context and sockets
    zmq::context_t context(1);
    // Catch startup exceptions; without this, startup errors are on stdout
    std::string zmq_socket = conf.zmq_socket_path();
    // TODO: try/catch
    LoadBalancer lb(context);

    const navitia::Metrics metrics(conf.metrics_binding(), conf.instance_name());

    threads.create_thread(navitia::MaintenanceWorker(data_manager, conf, metrics));
    //
    // Data have been loaded, we can now accept connections
    try {
        lb.bind(zmq_socket, "inproc://workers");
    } catch (zmq::error_t& e) {
        LOG4CPLUS_ERROR(logger, "zmq::socket_t::bind( " << zmq_socket << " ) failure: " << e.what());
        threads.interrupt_all();
        threads.join_all();
        return 1;
    }

    int nb_threads = conf.nb_threads();
    const std::string hostname = navitia::get_hostname();

    // Launch pool of worker threads
    LOG4CPLUS_INFO(logger, "starting workers threads");
    for (int thread_nbr = 0; thread_nbr < nb_threads; ++thread_nbr) {
        threads.create_thread([&context, &data_manager, conf, &metrics, &hostname, thread_nbr] {
            return doWork(context, data_manager, conf, metrics, hostname, thread_nbr);
        });
    }

    // Connect worker threads to client threads via a queue
    do {
        try {
            lb.run();
        } catch (const navitia::recoverable_exception& e) {
            LOG4CPLUS_ERROR(logger, e.what());
        } catch (const zmq::error_t&) {
        }  // lors d'un SIGHUP on restore la queue
    } while (true);
}
