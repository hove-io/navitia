#pragma once
#include <iosfwd>
#include <boost/iostreams/concepts.hpp>
#include <string.h>
#include <iostream>
#include "fastlz/fastlz.h"
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/cstdint.hpp>

typedef std::exception FlzException;


/**
 * Filtre de compression utilisant l'algorithme de compression fastlz pour boost::iostreams
 *
 */
class FastLZCompressor : public boost::iostreams::multichar_output_filter {
    std::streamsize buffer_size;
    char*  output_buffer;
public:
    
    /**
     * @param buffer_size correspond a la taille des chunk qui seront compressé, 
     * attention une valeur trop faible fait perdre en taux de compression et en vitesse
     *
     */
    FastLZCompressor(std::streamsize buffer_size=1024): buffer_size(buffer_size){
        output_buffer = new char[buffer_size];
    }

    FastLZCompressor(const FastLZCompressor& other) : buffer_size(other.buffer_size){
        output_buffer = new char[buffer_size];
    }


    ~FastLZCompressor(){
        delete[] output_buffer;
    }

    template<typename Sink>
    std::streamsize write(Sink& dest, const char* src, std::streamsize size){
        if(size > buffer_size){
            size = buffer_size;
        }
        uint32_t output_size;
        output_size = fastlz_compress(src, size, output_buffer);
        if(output_size > 0){
            boost::iostreams::write(dest, (char*)&output_size, sizeof(uint32_t));
            boost::iostreams::write(dest, output_buffer, output_size);
        }else{
            throw FlzException();
        }
        if(buffer_size < size){
            return buffer_size;
        }else{
            return size;
        }

    }
};

/**
 * Filtre de décompression utilisant l'algorithme de compression fastlz pour boost::iostreams
 *
 * le buffer de sortie doit etre dimensionné afin de pouvoir contenir un chunk décompréssé complet
 */
class FastLZDecompressor : public boost::iostreams::multichar_input_filter {
    std::streamsize buffer_size;
    char*  input_buffer;
public:

    /**
     * @param buffer_size correspond au buffer de travail pour la décompression, il doit etre suffisament grand pour 
     * intégralement contenir un chunk compréssé.
     *
     */
    FastLZDecompressor(std::streamsize buffer_size=1024): buffer_size(buffer_size){
        input_buffer = new char[buffer_size];
    }

    FastLZDecompressor(const FastLZDecompressor& other) : buffer_size(other.buffer_size){
        input_buffer = new char[buffer_size];
    }


    ~FastLZDecompressor(){
        delete[] input_buffer;
    }

    template<typename Source>
    std::streamsize read(Source& src, char* dest, std::streamsize size){
        std::streamsize output_size=0, read_size=0;
        uint32_t chunck_size=0;

        boost::iostreams::read(src, (char*)&chunck_size, sizeof(uint32_t));
        read_size = boost::iostreams::read(src, input_buffer, chunck_size);
        if(read_size == chunck_size){
            output_size = fastlz_decompress(input_buffer, read_size, dest, size);
        }else{
            if(chunck_size == 0 && read_size == -1){
                return -1;//le flux est vide
            }else{
                throw FlzException();
            }
        }
        return output_size;
    }
};

