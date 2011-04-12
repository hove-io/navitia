#pragma once
#include <iosfwd>
#include <boost/iostreams/concepts.hpp>
#include <string.h>
#include <iostream>
#include "fastlz.h"
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>



class FastLZCompressor : public boost::iostreams::multichar_output_filter {
    std::size_t buffer_size;
    char*  input_buffer;
    char*  output_buffer;
public:




    FastLZCompressor(size_t buffer_size=1024): buffer_size(buffer_size){
        input_buffer = new char[buffer_size];
        output_buffer = new char[buffer_size*2];
    }

    FastLZCompressor(const FastLZCompressor& other) : buffer_size(other.buffer_size){
        input_buffer = new char[buffer_size];
        output_buffer = new char[buffer_size*2];
    }


    ~FastLZCompressor(){
        ///@TODO trouver l'erreur
        delete[] input_buffer;
        delete[] output_buffer;
    }

    template<typename Sink>
    std::streamsize write(Sink& dest, const char* src, std::streamsize size){
        std::cout << "input: " << size << std::endl;
        std::streamsize output_size;
        output_size = fastlz_compress(src, size, output_buffer);
        std::cout << "output: " << output_size << std::endl;
        if(output_size > 0){
            boost::iostreams::write(dest, output_buffer, output_size);
        }
        return buffer_size;

    }
};


class FastLZDecompressor : public boost::iostreams::multichar_input_filter {
    std::streamsize buffer_size;
    std::streamsize output_buffer_size;
    char*  input_buffer;
    char*  output_buffer;
public:

    FastLZDecompressor(size_t buffer_size=1024): buffer_size(buffer_size){
        output_buffer_size = buffer_size * 2;
        input_buffer = new char[buffer_size];
        output_buffer = new char[output_buffer_size];
    }

    FastLZDecompressor(const FastLZDecompressor& other) : buffer_size(other.buffer_size){
        output_buffer_size = buffer_size * 2;
        input_buffer = new char[buffer_size];
        output_buffer = new char[buffer_size*2];
    }


    ~FastLZDecompressor(){
        delete[] input_buffer;
        delete[] output_buffer;
    }

    template<typename Source>
    std::streamsize read(Source& src, char* dest, std::streamsize size){
        std::cout << "decompression input: " << size << std::endl;
        std::streamsize output_size=0, read_size;
        
        read_size = boost::iostreams::read(src, input_buffer, buffer_size);
        if(read_size > 0){
            output_size = fastlz_decompress(input_buffer, read_size, dest, size);
        }else{
            std::cout << "rien de lu" << std::endl;
        }
        return output_size;
    }
};

