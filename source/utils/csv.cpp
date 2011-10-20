#include "csv.h"
#include <exception>
#include <boost/algorithm/string.hpp>

CsvReader::CsvReader(const std::string& filename, char separator, std::string encoding): filename(filename), file(filename), closed(false),
    functor('\\', separator, '"')
#ifdef HAVE_ICONV_H
	, converter(NULL)
#endif
{
    if(encoding != "UTF-8"){
        //TODO la taille en dur s'mal
#ifdef HAVE_ICONV_H
        converter = new EncodingConverter(encoding, "UTF-8", 2048);
#endif
    }
}

void CsvReader::close(){
    if(!closed){
        file.close();
#ifdef HAVE_ICONV_H
		//TODO géré des option de compile plutot que par plateforme
        delete converter;
        converter = NULL;
#endif
    }
}

bool CsvReader::eof() const{
    return file.eof();
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
#ifdef HAVE_ICONV_H
    if(converter != NULL){
        line = converter->convert(line);
    }
#endif
    std::vector<std::string> vec;
    Tokenizer tok(line, functor);
    vec.assign(tok.begin(), tok.end());

    return vec;
}
