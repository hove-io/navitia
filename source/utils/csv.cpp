#include "csv.h"
#include <exception>
#include <boost/algorithm/string.hpp>

CsvReader::CsvReader(const std::string& filename, char separator, std::string encoding): filename(filename), file(filename), closed(false),
    functor('\\', separator, '"'), converter(NULL)
{
    if(encoding != "UTF-8"){
        //TODO la taille en dur s'mal
        converter = new EncodingConverter(encoding, "UTF-8", 2048);
    }
}

void CsvReader::close(){
    if(!closed){
        file.close();
        delete converter;
        converter = NULL;
    }
}

std::vector<std::string> CsvReader::end(){
    return std::vector<std::string>();
}


CsvReader::~CsvReader(){
    this->close();
}

std::vector<std::string> CsvReader::next(){
    if(!this->file.is_open()){
        throw std::exception();
    }
    do{
        if(file.eof()){
            return std::vector<std::string>();
        }
        std::getline(file, line);
    }while(line.empty());
    boost::trim(line);
    if(converter != NULL){
        line = converter->convert(line);
    }
    std::vector<std::string> vec;
    Tokenizer tok(line, functor);
    vec.assign(tok.begin(), tok.end());

    return vec;
}
