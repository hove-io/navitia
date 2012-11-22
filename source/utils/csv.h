#pragma once

#include "config.h"

#include <string>
#include <fstream>
#include <vector>
#include <boost/tokenizer.hpp>
#ifdef HAVE_ICONV_H
#include "utils/encoding_converter.h"
#endif
#include <map>

/**
 * lecteur CSV basique, si iconv est disponible, le resultat serat retourné en UTF8
 */
class CsvReader {
    public:
        CsvReader(const std::string& filename, char separator=';', std::string encoding="UTF-8");
        ~CsvReader();
        std::vector<std::string> next();
        int get_pos_col(const std::string&, std::map<std::string, int>&);
        bool eof() const;
        void close();
    private:


        typedef boost::tokenizer<boost::escaped_list_separator<char> > Tokenizer;
        std::string filename;
        std::string line;
        std::ifstream file;
        bool closed;
        boost::escaped_list_separator<char> functor;
#ifdef HAVE_ICONV_H
        EncodingConverter* converter;
#endif


};

