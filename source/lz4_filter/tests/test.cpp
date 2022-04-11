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

#include "lz4_filter/filter.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_lz4_filter
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <string>

BOOST_AUTO_TEST_CASE(tiny_string_compression) {
    std::string str = "foo";
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(LZ4Compressor());
        out.push(boost::iostreams::file_sink("my_file.lz4"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(LZ4Decompressor());
        in.push(boost::iostreams::file_source("my_file.lz4"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}

BOOST_AUTO_TEST_CASE(string_compression) {
    std::string str =
        "foobariozafiozehfuiozefuigaezgfuzegfpuzheuerfhzeupgf_zéhefuçgzifgpzugfầi_zehufzeêfhzugfpeuzghfçpuzehfpuzghf";
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(LZ4Compressor());
        out.push(boost::iostreams::file_sink("my_file.lz4"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(LZ4Decompressor());
        in.push(boost::iostreams::file_source("my_file.lz4"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}

BOOST_AUTO_TEST_CASE(big_string_compression) {
    std::string str = "foobariozafiozehfuiozefuigaezgfuzegfpuzheuerfhzeupgf";
    for (int i = 0; i < 10; i++) {
        str += str;
    }
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(LZ4Compressor());
        out.push(boost::iostreams::file_sink("my_file.lz4"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(LZ4Decompressor());
        in.push(boost::iostreams::file_source("my_file.lz4"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}
