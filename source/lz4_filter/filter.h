#pragma once
#include <iosfwd>
#include <boost/iostreams/concepts.hpp>
#include <string.h>
#include <iostream>
#include "third_party/lz4/lz4.h"
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>
#include <boost/cstdint.hpp>

typedef std::exception LZ4Exception;


/**
 * Filtre de compression utilisant l'algorithme de compression LZ4 pour boost::iostreams
 *
 */
class LZ4Compressor : public boost::iostreams::multichar_output_filter {
    std::streamsize buffer_size;
    char*  output_buffer;
public:
    
    /**
     * @param buffer_size correspond a la taille des chunk qui seront compressé, 
     * attention une valeur trop faible fait perdre en taux de compression et en vitesse
     *
     */
    LZ4Compressor(std::streamsize buffer_size=1024): buffer_size(buffer_size){
        output_buffer = new char[buffer_size];
    }

    LZ4Compressor(const LZ4Compressor& other) : buffer_size(other.buffer_size){
        output_buffer = new char[buffer_size];
    }


    ~LZ4Compressor(){
        delete[] output_buffer;
    }

    template<typename Sink>
    std::streamsize write(Sink& dest, const char* src, std::streamsize size){
        if(size > buffer_size){
            size = buffer_size;
        }
        uint32_t output_size;
        output_size = LZ4_compress(src, output_buffer, size);
        if(output_size > 0){
            boost::iostreams::write(dest, (char*)&output_size, sizeof(uint32_t));
            boost::iostreams::write(dest, output_buffer, output_size);
        }else{
            throw LZ4Exception();
        }
        if(buffer_size < size){
            return buffer_size;
        }else{
            return size;
        }

    }
};

/**
 * Filtre de décompression utilisant l'algorithme de compression LZ4 pour boost::iostreams
 *
 * le buffer de sortie doit etre dimensionné afin de pouvoir contenir un chunk décompréssé complet
 */
class LZ4Decompressor : public boost::iostreams::multichar_input_filter {
    std::streamsize buffer_size;
    char*  input_buffer;
public:

    /**
     * @param buffer_size correspond au buffer de travail pour la décompression, il doit etre suffisament grand pour 
     * intégralement contenir un chunk compréssé.
     *
     */
    LZ4Decompressor(std::streamsize buffer_size=1024): buffer_size(buffer_size){
        input_buffer = new char[buffer_size];
    }

    LZ4Decompressor(const LZ4Decompressor& other) : buffer_size(other.buffer_size){
        input_buffer = new char[buffer_size];
    }


    ~LZ4Decompressor(){
        delete[] input_buffer;
    }

    template<typename Source>
    std::streamsize read(Source& src, char* dest, std::streamsize size){
        std::streamsize output_size=0, read_size=0;
        uint32_t chunk_size=0;

        boost::iostreams::read(src, (char*)&chunk_size, sizeof(uint32_t));
        read_size = boost::iostreams::read(src, input_buffer, chunk_size);
        if(read_size == chunk_size){
            output_size = LZ4_uncompress_unknownOutputSize(input_buffer, dest, chunk_size, size);
        }else{
            if(chunk_size == 0 && read_size == -1){
                return -1;//le flux est vide
            }else{
                throw LZ4Exception();
            }
        }
        return output_size;
    }
};

