#pragma once


#include <string>
#include <fstream>
#include <vector>
#include <boost/tokenizer.hpp>

class CsvReader {
    public:
        CsvReader (const std::string& filename, char separator=';');
        ~CsvReader ();
        std::vector<std::string> end();
        std::vector<std::string> next();
        void close();
    private:

        typedef boost::tokenizer<boost::escaped_list_separator<char> > Tokenizer;
        std::string filename;
        std::string line;
        std::ifstream file;
        bool closed;
        boost::escaped_list_separator<char> functor;


};

