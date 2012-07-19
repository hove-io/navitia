#pragma once

#include <iconv.h>
#include <string>

/// Classe permettant de convertir l'encodage de chaînes de caractères
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


/// Supprime le BOM s'il existe, il n'y a donc pas de risque à l'appeler tout seul
void remove_bom(std::fstream& stream);

