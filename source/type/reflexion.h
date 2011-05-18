#pragma once
#include <string>

/** Classe permettant la reflexion des données */


/// Énumération des attributs que l'on souhaite pouvoir accéder
enum member_e {
    id,
    idx,
    name,
    external_code,
    unknown_member_
};

member_e get_member(const std::string & member_str);

/// Énumération des classes
enum class_e {
    stop_area,
    stop_point,
    line,
    validity_pattern,
    unknown_class
};

class_e get_class(const std::string & class_str);

/// Série de spécialisations qui retourne l'élément souhaité
/// Il est créé car on ne peut pas spécialiser partiellement une fonction :/
template<class T, member_e E>
struct Reflective{};

template<class T>
struct Reflective<T,id> {
    typedef dectype(T::idx) type;
    static decltype(&T::id) ptr(){ return &T::id; }
};

template<class T>
struct Reflective<T,idx> {
    typedef decltype(T::idx) type;
    static decltype(&T::idx) ptr(){ return &T::idx; }
};

template<class T>
struct Reflective<T,name> {
    typedef decltype(T::name) type;
    static decltype(&T::name) ptr(){ return &T::name; }
};

template<class T>
struct Reflective<T,external_code> {
    typedef decltype(T::external_code) type;
    static decltype(&T::external_code) ptr(){ return &T::external_code;}
};



/// Wrapper qui permet d'accéder aux membres d'une classe T en fonction de l'attribut E
template<class T>
struct Members{
    template<member_e E>
    static decltype(Reflective<T, E>::ptr()) ptr(){return Reflective<T, E>::ptr();}

    template<member_e E>
    static typename Reflective<T, E>::type val(T& object){return object.*Reflective<T,E>::ptr();}
};
