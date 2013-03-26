#pragma once

#include <string>
#include <exception>

namespace navitia{
    class exception : public std::exception{
        protected:
        std::string msg;

        public:

        exception(const std::string& msg): msg(msg){}
        exception(){}

        virtual const char* what() const throw(){
            return msg.c_str();
        }

        virtual ~exception() throw (){}

    };
}

