#pragma once
#include <string>
#include <boost/variant.hpp>
#include <boost/utility/enable_if.hpp>
#include <iostream>

namespace navitia{ namespace ptref {
typedef boost::variant<std::string, int>  col_t;

/// Gestion des erreurs
struct PTRefException : public std::exception{
    // Constructeur de l'exception: msg représente le message, FunctionName représente la fonction
    //  qui a provoquée l'exception et FileName  le nom du fichier
    PTRefException(const std::string msg, std::string FunctionName, std::string FileName){
        this->msg = msg+", Fonction: ["+FunctionName+"], Fichier: ["+FileName+"]";
    }
    virtual ~PTRefException() throw(){}

    // Récupération du message
    virtual const char* what() const throw(){
        return this->msg.c_str();
    }
private:
    std::string msg;
};

/// Exception levée lorsqu'on demande un membre qu'on ne connait pas
struct unknown_member{};
struct bad_type{};

#define DECL_HAS_MEMBER(MEM_NAME) \
template <typename T> \
struct Reflect_##MEM_NAME { \
    typedef char yes[1]; \
    typedef char no[2]; \
    template <typename C> \
    static yes& test(decltype(&C::MEM_NAME)*); \
    template <typename> static no& test(...); \
    static const bool value = sizeof(test<T>(0)) == sizeof(yes); \
};\
template<class T> typename boost::enable_if<Reflect_##MEM_NAME<T>, decltype(T::MEM_NAME)>::type get_##MEM_NAME(T&object){return object.MEM_NAME;}\
template<class T> typename boost::disable_if<Reflect_##MEM_NAME<T>, int>::type get_##MEM_NAME(const T&){throw unknown_member();}\
template<class T> typename boost::enable_if<Reflect_##MEM_NAME<T>, decltype(T::MEM_NAME) T::*>::type ptr_##MEM_NAME(){return &T::MEM_NAME;}\
template<class T> typename boost::disable_if<Reflect_##MEM_NAME<T>, int T::*>::type ptr_##MEM_NAME(){throw unknown_member();}

DECL_HAS_MEMBER(external_code)
DECL_HAS_MEMBER(id)
DECL_HAS_MEMBER(idx)
DECL_HAS_MEMBER(name)
DECL_HAS_MEMBER(validity_pattern)
    
/// Wrapper pour éviter de devoir définir explicitement le type
template<class T>
col_t get_value(T& object, const std::string & name){
    if(name == "id") return get_id(object);
    else if(name == "idx") return get_idx(object);
    else if(name == "external_code") return get_external_code(object);
    else if(name == "name") return get_name(object);
    else
        throw unknown_member();
}

}}
