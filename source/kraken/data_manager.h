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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "utils/logger.h"
#include "utils/timer.h"
#include "type/data_exceptions.h"
#ifndef NO_FORCE_MEMORY_RELEASE
// by default we force the release of the memory after the reload of the data
#include "gperftools/malloc_extension.h"
#endif

#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include <mutex>
#include <memory>
#include <iostream>
#include <atomic>

template <typename Data>
void data_deleter(const Data* data) {
    delete data;
#ifndef NO_FORCE_MEMORY_RELEASE
    // we might want to force the system to release the memory after the destruction of data
    // to reduce the memory foot print
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
}

template <typename Data>
class DataManager {
    boost::shared_ptr<const Data> current_data;
    std::atomic_size_t data_identifier;

private:
    boost::shared_ptr<Data> create_data(size_t id) { return boost::shared_ptr<Data>(new Data(id), data_deleter<Data>); }

    boost::shared_ptr<const Data> create_ptr(const Data* d) {
        return boost::shared_ptr<const Data>(d, data_deleter<Data>);
    }
    bool load_data_nav(boost::shared_ptr<Data>& data, const std::string& filename) {
        try {
            data->load_nav(filename);
            return true;
        } catch (const navitia::data::data_loading_error&) {
            data->loading = false;
            return false;
        }
    }

    std::unique_ptr<std::mutex> mutex = std::make_unique<std::mutex>();

public:
    DataManager() : current_data(create_data(0)) { data_identifier = 0; }

    void set_data(const Data* d) { set_data(create_ptr(d)); }
    void set_data(boost::shared_ptr<const Data>&& data) {
        if (!data) {
            throw navitia::exception("Giving a null Data to DataManager::set_data");
        }
        data->is_connected_to_rabbitmq = current_data->is_connected_to_rabbitmq.load();
        current_data = std::move(data);
    }
    boost::shared_ptr<const Data> get_data() const {
        std::lock_guard<std::mutex> lock(*mutex);
        return current_data;
    }
    boost::shared_ptr<Data> get_data_clone() {
        ++data_identifier;
        auto data = create_data(data_identifier.load());
        time_it("Clone data: ", [&]() { data->clone_from(*current_data); });
        return std::move(data);
    }

    bool load(const std::string& filename,
              const boost::optional<std::string>& chaos_database = boost::none,
              const std::vector<std::string>& contributors = {},
              const size_t raptor_cache_size = 10) {
        // Add logger
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

        // Create new type::Data
        ++data_identifier;
        auto data = create_data(data_identifier.load());
        data->loading = true;

        // load .nav.lz4
        if (!load_data_nav(data, filename)) {
            if (data->last_load_succeeded) {
                LOG4CPLUS_INFO(logger, "Data loading failed, we keep last loaded data");
            }
            return false;
        }

        // load disruptions from database
        if (chaos_database != boost::none) {
            // If we catch a bdd broken connection, we do nothing (Just a log),
            // because data is still clean, unlike other cases where we have
            // to reload the data
            try {
                data->load_disruptions(*chaos_database, contributors);
                data->build_autocomplete_partial();
            } catch (const navitia::data::disruptions_broken_connection&) {
                LOG4CPLUS_WARN(logger, "Load data without disruptions");
            } catch (const navitia::data::disruptions_loading_error&) {
                // Reload data .nav.lz4
                LOG4CPLUS_ERROR(logger, "Reload data without disruptions: " << filename);
                data = create_data(data_identifier.load());
                if (!load_data_nav(data, filename)) {
                    LOG4CPLUS_ERROR(logger, "Reload data without disruptions failed...");
                    return false;
                }
            }
        }

        data->build_relations();
        // Build Raptor Data
        data->build_raptor(raptor_cache_size);
        // Build proximity list NN index
        data->build_proximity_list();
        data->loading = false;

        // Set data
        set_data(std::move(data));

        return true;
    }
};
