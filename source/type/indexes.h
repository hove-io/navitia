#pragma once

#include <boost/iterator/transform_iterator.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/foreach.hpp>

#include <boost/iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/regex.hpp>
#include <boost/shared_container_iterator.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

/** Functor permettant de tester l'attribut passé en paramètre avec la valeur passée en paramètre
     *
     * par exemple is_equal<std::string>(&StopPoint::name, "Châtelet");
     */
template<class Attribute, class T>
struct is_equal_t{
    Attribute ref;
    Attribute T::*attr;
    is_equal_t(Attribute T::*attr, const Attribute & ref) : ref(ref), attr(attr){}
    bool operator()(const T & elt) const {return ref == elt.*attr;}
};
template<class Attribute, class T>
is_equal_t<Attribute, T> is_equal(Attribute T::*attr, const Attribute & ref){
    return is_equal_t<Attribute, T>(attr, ref);
}

/** Functor qui matche une expression rationnelle sur l'attribut passé en paramètre
     *
     * L'attribut doit être une chaîne de caractères
     */
template<class T>
struct matches {
    const boost::regex e;
    std::string T::*attr;
    matches(std::string T::*attr, const std::string & e) : e(e), attr(attr){}
    bool operator()(const T & elt) const {return boost::regex_match(elt.*attr, e );}
};

template<class T1, class A1, class T2, class A2>
struct attribute_equals_t {
    A1 T1::*attr1;
    A2 T2::*attr2;
    attribute_equals_t(A1 T1::*attr1, A2 T2::*attr2) : attr1(attr1), attr2(attr2) {}
    bool operator()(const T1 & t1, const boost::tuple<T2> & t2){ return t1.*attr1 == t2.*attr2;}
    bool operator()(const T1 & t1, const boost::tuple<T2*> & t2){ return t1.*attr1 == (*(boost::get<0>(t2))).*attr2;}
};
template<class T1, class A1, class T2, class A2>
attribute_equals_t<T1, A1, T2, A2> attribute_equals(A1 T1::*attr1, A2 T2::*attr2){
    return attribute_equals_t<T1, A1, T2, A2>(attr1, attr2);
}

template<class T, class A>
struct attribute_cmp_t {
    A T::*attr;
    attribute_cmp_t(A T::*attr) : attr(attr) {}
    bool operator()(const T & t1, const T & t2){ return t1.*attr <t2.*attr;}
};
template<class T, class A>
attribute_cmp_t<T,A> attribute_cmp(A T::*attr){return attribute_cmp_t<T,A>(attr);}

template<int N, class T> typename boost::remove_pointer<typename boost::tuples::element<N,T>::type>::type & join_get(T & t) {return *(boost::get<N>(t));}

template<class Iter>
class Subset {
    Iter begin_it;
    Iter end_it;
public:

    typedef typename Iter::value_type value_type;
    typedef typename Iter::pointer pointer;
    typedef Iter iterator;
    typedef Iter const_iterator;
    Subset(const Iter & begin_it, const Iter & end_it) :  begin_it(begin_it), end_it(end_it) {}
    iterator begin() {return begin_it;}
    iterator end() {return end_it;}
    const_iterator begin() const {return begin_it;}
    const_iterator end() const {return end_it;}

    template<class Functor>
    Subset<boost::filter_iterator<Functor, iterator> >
            filter(Functor f) const{
        return Subset<boost::filter_iterator<Functor, iterator> >
                (   boost::make_filter_iterator(f, begin(), end()),
                    boost::make_filter_iterator(f, end(), end())
                    );
    }

    /** Filtre selon la valeur d'un attribut qui matche une regex */
    Subset<boost::filter_iterator<matches<value_type>, iterator> >
            filter_match(std::string value_type::*attr, const std::string & str ) {
        return filter(matches<value_type>(attr, str));
    }

    /** Filtre selon la valeur d'un attribut */
    template<class Attribute, class Param>
    Subset<boost::filter_iterator<is_equal_t<Attribute, value_type>, iterator> >
            filter(Attribute value_type::*attr, Param value ) {
        return filter(is_equal<Attribute, value_type>(attr, Param(value)));
    }

    /// Functor permettant de trier les données
    template<class Functor>
    struct Sorter{
        Iter begin_it;
        Functor f;

        Sorter(Iter begin_it, Functor f) : begin_it(begin_it), f(f){}

        bool operator()(size_t a, size_t b){
            return f(*(begin_it + a), *(begin_it + b));
        }
    };

    template<class Functor>
    Subset<boost::permutation_iterator<iterator, boost::shared_container_iterator< std::vector<ptrdiff_t> > > >
            order(Functor f){
        boost::shared_ptr< std::vector<ptrdiff_t> > indexes(new std::vector<ptrdiff_t>(end_it - begin_it));

        for(size_t i=0; i < indexes->size(); i++) {
            (*indexes)[i] = i;
        }

        typedef boost::shared_container_iterator< std::vector<ptrdiff_t> > shared_iterator;
        auto a = shared_iterator(indexes->begin(), indexes);

        auto b = shared_iterator(indexes->end(), indexes);
        sort(indexes->begin(), indexes->end(), Sorter<Functor>(begin_it, f));
        return Subset<boost::permutation_iterator<iterator, shared_iterator> >
                ( boost::make_permutation_iterator(begin(), a),
                  boost::make_permutation_iterator(begin(), b)
                  );
    }
};

template<class Iter>
Subset<Iter> make_subset(Iter begin, Iter end) {
    return Subset<Iter>(begin, end);
}

template<class Container>
Subset<typename Container::iterator> make_subset(Container & c) {
    return make_subset(c.begin(), c.end());
}

template<class Container, class Functor>
Subset<typename boost::filter_iterator<Functor, typename Container::iterator> >
        filter(typename Container::iterator begin, typename Container::iterator end, Functor f) {
    auto subset = make_subset(begin, end);
    return subset.filter(f);
}

template<class Container, class Functor>
Subset<typename boost::filter_iterator<Functor, typename Container::iterator> > filter(Container & c, Functor f) {
    auto subset = make_subset(c.begin(), c.end());
    return subset.filter(f);
}

template<class Container, class Attribute>
Subset<typename boost::filter_iterator<is_equal_t<Attribute, typename Container::value_type>,typename Container::iterator > >
        filter(Container & c, Attribute Container::value_type::*attr, const Attribute & val) {
    auto f = is_equal(attr, val);
    return filter(c, f);
}

template<class Container, class Attribute>
Subset<typename boost::permutation_iterator<typename Container::iterator, typename boost::shared_container_iterator<typename std::vector<ptrdiff_t> > > >
      order(Container & c, Attribute Container::value_type::*attr){
    auto tmp = make_subset(c);
    return tmp.order(attribute_cmp(attr));
}

template<class Type>
class Index {
    typedef typename Type::iterator::value_type value_type;
    typedef value_type type;

    struct Transformer{
        typedef value_type & result_type;
        value_type* begin;

        Transformer(value_type * begin){this->begin = begin;}

        value_type & operator()(int diff) const {
            return *(begin + diff);
        }
        
    };

    value_type* begin_it;

    std::vector<int> indexes;

public:
    typedef typename boost::transform_iterator<Transformer, typename std::vector<int>::iterator> iterator;
    typedef typename boost::transform_iterator<Transformer, typename std::vector<int>::const_iterator> const_iterator;
    
    Index(typename Type::iterator begin, typename Type::iterator end) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(typename Type::value_type & element, std::make_pair(begin, end)) {
            indexes.push_back(&element - this->begin_it);//on stock la différence entre les deux pointeurs
        }
    }

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}
    const_iterator begin() const {return const_iterator(indexes.begin(), Transformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), Transformer(begin_it));}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes;
    }
};
/*
template<class Type>
class JoinIndex {
    typedef typename Type::iterator::value_type value_type;
    typedef value_type type;

    struct Transformer{
        typedef value_type & result_type;
        value_type* begin;

        Transformer(value_type * begin){this->begin = begin;}

        value_type & operator()(int diff) const {
            return *(begin + diff);
        }

    };

    //value_type* begin_it;

    std::vector<int> indexes;

    public:
    typedef typename boost::transform_iterator<Transformer, typename std::vector<int>::iterator> iterator;
    typedef typename boost::transform_iterator<Transformer, typename std::vector<int>::const_iterator> const_iterator;
   // typedef typename boost::indirect_iterator<typename std::vector<pointer>::const_iterator > const_iterator;

    Index(typename Type::iterator begin, typename Type::iterator end) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(typename Type::value_type & element, std::make_pair(begin, end)) {
            indexes.push_back(&element - this->begin_it);//on stock la différence entre les deux pointeurs
        }
    }

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}
    const_iterator begin() const {return const_iterator(indexes.begin(), Transformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), Transformer(begin_it));}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes;
    }
 };
*/
template<class Type>
Index<Type>
        make_index(typename Type::iterator begin, typename Type::iterator end) {
    return Index<Type>(begin, end);
}

template<class Type>
Index<Type> make_index(const Type & t) {
    return Index<Type>(t.begin(), t.end());
}

template<class Head, class Tail, class Functor>
class join_iterator{
public:
    typedef typename boost::tuples::cons<typename Head::value_type*, typename Tail::value_type> value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category; 
    typename Head::iterator begin1, current1, end1;

    Tail begin2, current2, end2;
    Functor f;

    join_iterator(typename Head::iterator c1_begin, typename Head::iterator c1_end,
                  Tail c2_begin, Tail c2_end,
                  const Functor & f) :
    begin1(c1_begin), current1(c1_begin), end1(c1_end),
    begin2(c2_begin), current2(c2_begin), end2(c2_end),
    f(f)
    {increment();}

    value_type dereference() const { return value_type(&(*current1), *current2);}

    bool equal(const join_iterator & other) const { return other.current1 == current1 && other.current2 == current2;}

    void increment(){
        current2++;//on passe a l'élément suivant, sinon on reste toujours bloqué sur la premiére solution
        //fix: si current2 == end2 ?
        for(; current1 != end1; ++current1) {
            for(; current2 != end2; ++current2) {
                if(f(*current1, *current2))
                    return;
            }
            current2 = begin2;
        }
        current2 = end2;
    }

    value_type operator*() const {
        return dereference();
    }

    bool operator==(const join_iterator & other) const {
        return equal(other);
    }

    bool operator!=(const join_iterator & other) const {
        return !equal(other);
    }

    void operator++(int){
        increment();
    }

    value_type operator++(){
        increment();
        return dereference();
    }
};

template<class Head>
class join_iterator<Head, boost::tuples::null_type, boost::tuples::null_type> {
    typename Head::iterator begin, current, end;

public:
    typedef boost::tuple<typename Head::value_type*> value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category;
    join_iterator(typename Head::iterator begin, typename Head::iterator end):
            begin(begin), current(begin), end(end)//, f(f)
    {}

    value_type operator*() const{
        return boost::make_tuple(&(*current));
    }

    join_iterator & operator++(){
        ++current;
        return *this;
    }

    bool operator==(const join_iterator & other) const {
        return current == other.current;
    }

    bool operator!=(const join_iterator & other) const {
        return current != other.current;
    }

    void operator++(int){
        ++current;
    }
};

template<class Container1, class Container2, class Functor>
Subset<join_iterator<Container1, join_iterator<Container2, boost::tuples::null_type, boost::tuples::null_type>, Functor> >
        make_join(Container1 & c1, Container2 & c2, const Functor & f) {
    typedef join_iterator<Container2, boost::tuples::null_type, boost::tuples::null_type> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Container1, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Container1, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}


template<class Container>
Subset<join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type> >
        make_join(Container & c) {
    return make_subset (
            join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type>(c.begin(), c.end()),
            join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type>(c.end(), c.end())
            );
}
