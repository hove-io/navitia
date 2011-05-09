#include "filter.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_fastlz_filter
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <string>


BOOST_AUTO_TEST_CASE(tiny_string_compression){
    std::string str = "foo";
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(FastLZCompressor());
        out.push(boost::iostreams::file_sink("my_file.flz"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(FastLZDecompressor());
        in.push(boost::iostreams::file_source("my_file.flz"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}


BOOST_AUTO_TEST_CASE(string_compression){
    std::string str = "foobariozafiozehfuiozefuigaezgfuzegfpuzheuerfhzeupgf_zéhefuçgzifgpzugfầi_zehufzeêfhzugfpeuzghfçpuzehfpuzghf";
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(FastLZCompressor());
        out.push(boost::iostreams::file_sink("my_file.flz"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(FastLZDecompressor());
        in.push(boost::iostreams::file_source("my_file.flz"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}



BOOST_AUTO_TEST_CASE(big_string_compression){
    std::string str = "foobariozafiozehfuiozefuigaezgfuzegfpuzheuerfhzeupgf";
    for (int i = 0; i < 10; i++) {
        str += str;
    }
    std::string result;
    {
        boost::iostreams::filtering_ostream out;
        out.push(FastLZCompressor());
        out.push(boost::iostreams::file_sink("my_file.flz"));
        out << str;
    }
    {
        boost::iostreams::filtering_istream in;
        in.push(FastLZDecompressor());
        in.push(boost::iostreams::file_source("my_file.flz"));
        in >> result;
    }
    BOOST_CHECK_EQUAL(str, result);
}
