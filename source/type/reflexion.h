#pragma once
#include <string>
#include <boost/variant.hpp>
typedef boost::variant<std::string, int>  col_t;

/// Exception levée lorsqu'on demande un membre qu'on ne connait pas
struct unknown_member{};

/** Classe permettant la reflexion des données */


/// Énumération des attributs que l'on souhaite pouvoir accéder
enum member_e {
    id,
    idx,
    name,
    external_code,
    validity_pattern,
    unknown_member_
};

member_e get_member(const std::string & member_str);
/*
/// Énumération des classes
enum class_e {
    stop_area,
    stop_point,
    line,
    validity_pattern,
    unknown_class
};
*/
//class_e get_class(const std::string & class_str);

/// Série de spécialisations qui retourne l'élément souhaité
/// Il est créé car on ne peut pas spécialiser partiellement une fonction :/
template<class T, member_e E>
struct Reflective{typedef void type; static void ptr(){throw unknown_member();}};

template<class T>
struct Reflective<T,id> {
    typedef decltype(T::id) type;
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

template<class T>
struct Reflective<T,validity_pattern> {
    typedef decltype(T::validity_pattern) type;
    static decltype(&T::validity_pattern) ptr(){ return &T::validity_pattern;}
};

template<class T>
struct Reflective<T,unknown_member_> {
    typedef decltype(T::unknown_member_) type;
    static decltype(&T::unknown_member_) ptr(){ return &T::unknown_member_;}
};

//template<typename U> typename Reflective<T, E>::type test( U ) {return 'a';}
//template<typename U> int test(...) {return 42;}

/// Wrapper qui permet d'accéder aux membres d'une classe T en fonction de l'attribut E
template<class T>
struct Members{
    template<member_e E>
    static decltype(Reflective<T, E>::ptr()) ptr(){return Reflective<T, E>::ptr();}

    template<member_e E>
    static typename Reflective<T, E>::type val(T& object){return object.*Reflective<T,E>::ptr();}

    //template<member_e E>
    //static int val(T& object, char = 0){return -1;}

   // template<member_e E>
    //static int val(T& object){return -1;}

    static col_t value(T& object, const std::string & m_name){
        if(m_name == "id") return val<id>(object);
        else if(m_name == "idx") return val<idx>(object);
        else if(m_name == "external_code") return val<external_code>(object);
        else if(m_name == "name") return val<name>(object);
      //  else if(m_name == "validity_pattern") return val<validity_pattern>(object);
        else
            throw unknown_member();
    }
};

/// Wrapper pour éviter de devoir définir explicitement le type
template<class T>
col_t get_value(T& object, const std::string & name){
    return Members<T>::value(object, name);
}
