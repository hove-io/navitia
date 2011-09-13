#pragma once

#include <iconv.h>
#include <string>

class EncodingConverter{
    public:
        EncodingConverter(std::string from, std::string to, size_t buffer_size);
        std::string convert(std::string& str);
        virtual ~EncodingConverter();
    private:
        iconv_t iconv_handler;
        char* iconv_input_buffer;
        char* iconv_output_buffer;
        size_t buffer_size;


};

void remove_bom(std::fstream& stream);

