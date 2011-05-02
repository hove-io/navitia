#include "csv.h"
#include <exception>

CsvReader::CsvReader(const std::string& filename, char separator): filename(filename), file(filename), closed(false),
    functor('\\', separator, '"')
{}

void CsvReader::close(){
    if(!closed){
        file.close();
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
    std::vector<std::string> vec;
    Tokenizer tok(line, functor);
    vec.assign(tok.begin(), tok.end());
    return vec;
}
