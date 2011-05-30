#include "encoding_converter.h"
#include <string.h>

EncodingConverter::EncodingConverter(std::string from, std::string to, size_t buffer_size) : buffer_size(buffer_size){
    iconv_handler = iconv_open(to.c_str(), from.c_str());
    iconv_input_buffer = new char[buffer_size];
    iconv_output_buffer = new char[buffer_size];
}


std::string EncodingConverter::convert(std::string& str){
    memset(iconv_output_buffer, 0, buffer_size);
    strncpy(iconv_input_buffer, str.c_str(), buffer_size);
    char* working_input = iconv_input_buffer;
    char* working_output = iconv_output_buffer;
    size_t output_left = buffer_size;
    size_t input_left = str.size();
    size_t result = iconv(iconv_handler, &working_input, &input_left, &working_output, &output_left);
    if(result == -1){
        throw std::string("iconv fail: " + errno);
    }
    return std::string(iconv_output_buffer);

}


EncodingConverter::~EncodingConverter(){
    delete[] iconv_output_buffer;
    delete[] iconv_input_buffer;
    iconv_close(iconv_handler);

}
