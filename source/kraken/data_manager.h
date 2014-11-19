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

#pragma once

#include "utils/timer.h"
#include "utils/exception.h"
#ifndef NO_FORCE_MEMORY_RELEASE
//by default we force the release of the memory after the reload of the data
#include "gperftools/malloc_extension.h"
#endif

#include <memory>
#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

template<typename Data>
class DataManager{
    boost::shared_ptr<const Data> current_data;
public:

    DataManager() : current_data(boost::make_shared<const Data>()){}

    void set_data(const Data* d) { set_data(boost::shared_ptr<const Data>(d)); }
    void set_data(boost::shared_ptr<const Data>&& data) {
        if (!data) { throw navitia::exception("Giving a null Data to DataManager::set_data"); }
        data->is_connected_to_rabbitmq = current_data->is_connected_to_rabbitmq.load();
        current_data = std::move(data);
        release_memory();
    }
    boost::shared_ptr<const Data> get_data() const { return current_data; }
    boost::shared_ptr<Data> get_data_clone() const {
        auto data = boost::make_shared<Data>();
        time_it("Clone data: ", [&]() { data->clone_from(*current_data); });
        return std::move(data);
    }

    void release_memory(){
#ifndef NO_FORCE_MEMORY_RELEASE
        //we might want to force the system to release the memory after the swap
        //to reduce the memory foot print
        MallocExtension::instance()->ReleaseFreeMemory();
#endif
    }

    bool load(const std::string& database, 
              const boost::optional<std::string>& chaos_database = {}){
        bool success;
        auto data = boost::make_shared<Data>();
        success = data->load(database, chaos_database);
        if (success) {
            set_data(std::move(data));
        } else {
            release_memory();
        }
        return success;
    }
};
