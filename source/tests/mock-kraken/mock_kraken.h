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

#pragma once
#include "type/data.h"
#include "kraken/data_manager.h"
#include "kraken/kraken_zmq.h"

#include "ed/build_helper.h"
#include <zmq.hpp>
#include "utils/zmq.h"
#include <boost/thread.hpp>
#include "kraken/configuration.h"
#include <boost/program_options.hpp>
#include <boost/optional.hpp>

namespace po = boost::program_options;

/**
 * Pratically the same behavior as a real kraken, but with already loaded data
 */
struct mock_kraken {
    inline mock_kraken(ed::builder& d, int argc, const char* const argv[]) {
        d.finish();
        DataManager<navitia::type::Data> data_manager;
        data_manager.set_data(d.data.release());

        boost::thread_group threads;
        // Prepare our context and sockets
        zmq::context_t context(1);

        // we load the conf to have the default values
        navitia::kraken::Configuration conf;
        // we mock a command line load
        po::options_description desc =
            navitia::kraken::get_options_description(boost::optional<std::string>("default"),
                                                     boost::none,  // zmq_socket is set throught cmd line option
                                                     boost::optional<bool>(true));  // not used
        auto other_options = conf.load_from_command_line(desc, argc, argv);

        LoadBalancer lb(context);
        lb.bind(conf.zmq_socket_path(), "inproc://workers");
        navitia::Metrics metric(boost::none, "mock");

        // this option is not parsed by get_options_description because it is used only here
        if (std::find(other_options.begin(), other_options.end(), "spawn_maintenance_worker") != other_options.end()) {
            threads.create_thread(navitia::MaintenanceWorker(data_manager, conf, metric));
        }

        // Launch only one thread for the tests
        threads.create_thread([&context, &data_manager, conf, &metric] {
            return doWork(context, data_manager, conf, metric, "myhostname", 0);
        });

        // Connect work threads to client threads via a queue
        do {
            try {
                lb.run();
            } catch (const zmq::error_t&) {
            }  // lors d'un SIGHUP on restore la queue
        } while (true);
    }
};
