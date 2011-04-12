#include <fstream>
#include "filter.h"
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/invert.hpp>

int main (int argc, char const* argv[])
{

    char * str;
    std::string foo;
    str = new char[129];
    for(int i=0; i<129; i++){
        str[i] = 'a';
    }
    str[128] = '\0';
    {
        boost::iostreams::filtering_ostream out;
        out.push(FastLZCompressor(128));
        out.push(boost::iostreams::file_sink("my_file.txt"));
        std::cout << str << std::endl;
        out << str;
    }
    std::cout << "decompression" << std::endl;
    {
        boost::iostreams::filtering_istream in;
        in.push(FastLZDecompressor(128));
        in.push(boost::iostreams::file_source("my_file.txt"));
        in >> foo;
        std::cout << foo << std::endl;
    }

    return 0;
}
