#pragma once

/** Classe permettant la reflexion des données */

struct not_a_member : public std::exception {};

/// Énumération des attributs que l'on souhaite pouvoir accéder
enum member_e {
    id,
    idx,
    name,
    external_code
};

/// Série de spécialisations qui retourne l'élément souhaité
/// Il est créé car on ne peut pas spécialiser partiellement une fonction :/
template<class T, member_e E>
struct Reflective{};

template<class T>
struct Reflective<T,id> {static decltype(&T::id) get(){ return &T::id; }};

template<class T>
struct Reflective<T,idx> {static decltype(&T::idx) get(){ return &T::idx; }};

template<class T>
struct Reflective<T,name> {static decltype(&T::name) get(){ return &T::name; }};

template<class T>
struct Reflective<T,external_code> {static decltype(&T::external_code) get(){ return &T::external_code; }};

/// Wrapper qui permet d'accéder aux membres d'une classe T en fonction de l'attribut E
template<class T>
struct Members{
    template<member_e E>
    static decltype(Reflective<T, E>::get()) get(){return Reflective<T, E>::get();}
};
