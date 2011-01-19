#pragma once

#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/regex.hpp>
#include <boost/shared_container_iterator.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/view/reverse_view.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/adapted/array.hpp>
#include <boost/utility/result_of.hpp>

/** Fonction permettant d'accéder à l'élément N d'un tuple de jointure*/
template<int N, class T> typename boost::remove_pointer<typename boost::tuples::element<N,T>::type>::type & join_get(T & t){
    return *(boost::get<N>(t));
}

template<class E, class H, class T> typename boost::remove_pointer<E>::type &
join_get(typename boost::tuples::cons<H,T> tuple){
    return join_get<E>(tuple.get_tail());
}

template<class E, class T> typename boost::remove_pointer<E>::type &
join_get(typename boost::tuples::cons<E*,T> tuple){
    return *(tuple.get_head());
}

/** Functor permettant de tester l'attribut passé en paramètre avec la valeur passée en paramètre
  *
  * par exemple is_equal<std::string>(&StopPoint::name, "Châtelet");
  */
template<class Attribute, class T>
struct is_equal_t{
    Attribute ref;
    Attribute T::*attr;
    is_equal_t(Attribute T::*attr, const Attribute & ref) : ref(ref), attr(attr){}
    bool operator()(const T & elt) const {
        Attribute value = elt.*attr;    
        return ref == value;
    }
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
    template<class Tuple>
    bool operator()(const T1 & t1, const Tuple & t2){ return t1.*attr1 == join_get<T2>(t2).*attr2;}
    template<class Tuple1, class Tuple2>
    bool operator()(const Tuple1 & t1, const Tuple2 & t2){return join_get<T1>(t1).*attr1 == join_get<T2>(t2).*attr2;}
    //bool operator()()
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
    typedef Type value_type;

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
    
    Index(){}
    template<class Iterator>
    Index(Iterator begin, Iterator end) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(value_type & element, std::make_pair(begin, end)) {
            indexes.push_back(&element - this->begin_it);//on stock la différence entre les deux pointeurs
        }
    }

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}
    const_iterator begin() const {return const_iterator(indexes.begin(), Transformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), Transformer(begin_it));}

    value_type & operator[](int idx){
        return *(begin_it + idx);
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & begin_it;
    }
};

template<class Type>
class StringIndex {
    typedef Type value_type;

    typedef std::map<std::string, int> map_t;

    struct Transformer{
        typedef value_type & result_type;
        typedef result_type result;
        value_type* begin;

        Transformer(value_type * begin){this->begin = begin;}

        value_type & operator()(typename map_t::value_type elem) const {
            return *(begin + elem.second);
        }
    };

    value_type* begin_it;

    map_t indexes;

public:
    typedef typename boost::transform_iterator<Transformer, map_t::iterator> iterator;
    typedef typename boost::transform_iterator<Transformer, map_t::const_iterator> const_iterator;

    StringIndex() {}
    template<class Iterator>
    StringIndex(Iterator begin, Iterator end, typename std::string value_type::* attr) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(value_type & element, std::make_pair(begin, end)) {
            indexes[element.*attr] = &element - this->begin_it;//on stock la différence entre les deux pointeurs
        }
    }

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}
    const_iterator begin() const {return const_iterator(indexes.begin(), Transformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), Transformer(begin_it));}

    value_type & operator[](const std::string & key){
        map_t::iterator it = indexes.find(key);
        if(it == indexes.end()){
            throw std::out_of_range("Key not found");
        }
        return *(begin_it + it->second);
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & begin_it;
    }
};

namespace boost {
    namespace fusion {
        template<class Archive>
        struct serializer {
            Archive & ar;
            serializer(Archive & ar) : ar(ar) {}
            template <typename T>
            void operator()(T & data) const {
                ar & data;
            }
        };

        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9, typename Archive>
        void serialize(Archive & ar, vector<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
    }
}

template<class Type>
class JoinIndex {
    typedef Type value_type;
    typedef typename boost::fusion::result_of::as_vector<value_type>::type begin_it_t;
    typedef boost::array<int, boost::tuples::length<value_type>::value> idx_tuple;
    typedef typename boost::fusion::result_of::as_vector<idx_tuple>::type idx_vector;

    begin_it_t begin_it;
    std::vector<idx_vector> indexes;

    struct Transformer{
        typedef value_type result_type;
        begin_it_t begin;

        struct as_fold_t{
            typedef value_type result_type;
            template<class H, class T>
            boost::tuples::cons<H, T> operator()(const T & tail, const H & head) const {
                return boost::tuples::cons<H, T>(head, tail);
            }
        };

        struct get_pointer{
            template <typename Sig> struct result;

            template <typename T1, typename T2>
            struct result<get_pointer(T1, T2)> {typedef T1 type; };

            template<class T1, class T2>
            T1 operator()(T1 begin, T2 position) const {return begin + position;}
        };

        Transformer(begin_it_t begin){this->begin = begin;}

        value_type operator()(idx_vector diff) const {
            auto result_view = boost::fusion::transform(begin, diff, get_pointer());
            auto result_vec = boost::fusion::as_vector(boost::fusion::reverse(result_view));
            return boost::fusion::fold(result_vec, typename boost::tuples::null_type(), as_fold_t());
        }
        template<class Archive> void serialize(Archive & ar, const unsigned int ) {
            ar & indexes & begin_it;
        }

    };


    public:
    typedef typename boost::transform_iterator<Transformer, typename std::vector<idx_vector>::iterator> iterator;
    typedef typename boost::transform_iterator<Transformer, typename std::vector<idx_vector>::const_iterator> const_iterator;

    struct ptr_diff{
        typedef int result_type;
        template<class T1>
        int operator()(const T1 * begin, const T1 * current) const {return current - begin;}
    };


    JoinIndex(){}

    template<class Iterator>
    JoinIndex(Iterator begin, Iterator end) {
        this->begin_it = *begin;
        BOOST_FOREACH(const value_type & element, std::make_pair(begin, end)) {
            auto moo = boost::fusion::transform(begin_it, element, ptr_diff());
         //   auto moo2 = boost::fusion::as_vector(moo);
            //indexes.push_back(&element - this->begin_it);//on stock la différence entre les deux pointeurs
            indexes.push_back(moo);
            //std::cout << boost::fusion::at_c<0>(moo) << " " << boost::fusion::at_c<1>(moo)  << std::endl;
        }
    }

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}
    const_iterator begin() const {return const_iterator(indexes.begin(), Transformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), Transformer(begin_it));}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & begin_it;
    }
 };


template<class Type>
Index<typename Type::value_type> make_index(const Type & t) {
    return Index<typename Type::value_type>(t.begin(), t.end());
}

template<class Type>
StringIndex<typename Type::value_type> make_string_index(Type & t, typename std::string Type::value_type::*attr) {
    return StringIndex<typename Type::value_type>(t.begin(), t.end(), attr);
}

template<class Type>
JoinIndex<typename Type::value_type> make_join_index(Type & t){
    return JoinIndex<typename Type::value_type>(t.begin(), t.end());
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
    f(f) {
        if(begin1 == end1 || begin2 == end2){
            current1 = end1;
            current2 = end2;
            return;
        }
        if(!this->f(*current1, *current2))
            increment();
    }

    value_type dereference() const { return value_type(&(*current1), *current2);}

    bool equal(const join_iterator & other) const { return other.current1 == current1 && other.current2 == current2;}

    // Deux manières de sortir de la fonction :
    // 1) on a atteint la fin (current1==end1 && current2==end2)
    // 2) on a trouvé un cas qui valide le prédicat
    void increment(){
        ++current2;//on passe a l'élément suivant, sinon on reste toujours bloqué sur la premiére solution
        if(current2 == end2) {
            ++current1;
            if(current1 == end1) {
                return;
            }
            else {
                current2 = begin2;
            }
        }
        if(!f(*current1, *current2)) {
            increment();
        }
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
    typedef boost::tuples::cons<typename Head::value_type*, boost::tuples::null_type> value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category;
    join_iterator(typename Head::iterator begin, typename Head::iterator end):
            begin(begin), current(begin), end(end)//, f(f)
    {}

    value_type operator*() const{
        typename Head::value_type & result = *current;
        return value_type(&result, boost::tuples::null_type());
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


template<class Container1, class Container2, class Functor>
Subset<join_iterator<Container1, join_iterator<Subset<Container2>, boost::tuples::null_type, boost::tuples::null_type>, Functor> >
        make_join(Container1 & c1, Subset<Container2> c2, const Functor & f) {
    typedef join_iterator<Subset<Container2>, boost::tuples::null_type, boost::tuples::null_type> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Container1, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Container1, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}

template<class Container1, class H, class T, class F, class Functor>
Subset<join_iterator<Container1, join_iterator<H,T,F>, Functor> >
        make_join(Container1 & c1, Subset<join_iterator<H,T,F> > c2, const Functor & f) {

    return make_subset(
            join_iterator<Container1, join_iterator<H,T,F>, Functor>(c1.begin(), c1.end(), c2.begin(), c2.end(), f),
            join_iterator<Container1, join_iterator<H,T,F>, Functor>(c1.end(), c1.end(), c2.end(), c2.end(), f)
            );
}


template<class Container1, class Container2, class Functor>
Subset<join_iterator<Subset<Container1>, join_iterator<Container2, boost::tuples::null_type, boost::tuples::null_type>, Functor> >
        make_join(Subset<Container1> c1, Container2 & c2, const Functor & f) {
    typedef join_iterator<Container2, boost::tuples::null_type, boost::tuples::null_type> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}


template<class Container1, class Container2, class Functor>
Subset<join_iterator<Subset<Container1>, join_iterator<Subset<Container2>, boost::tuples::null_type, boost::tuples::null_type>, Functor> >
        make_join(Subset<Container1> c1, Subset<Container2> c2, const Functor & f) {
    typedef join_iterator<Subset<Container2>, boost::tuples::null_type, boost::tuples::null_type> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}

template<class Container>
Subset<join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type> >
        make_join(Container & c) {
    auto result = make_subset (
            join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type>(c.begin(), c.end()),
            join_iterator<Container, boost::tuples::null_type, boost::tuples::null_type>(c.end(), c.end())
            );
    return result;
}

template<class Container>
Subset<join_iterator<Subset<Container>, boost::tuples::null_type, boost::tuples::null_type> >
        make_join(Subset<Container> ss){
    return make_subset (
                join_iterator<Subset<Container>, boost::tuples::null_type, boost::tuples::null_type>(ss.begin(), ss.end()),
                join_iterator<Subset<Container>, boost::tuples::null_type, boost::tuples::null_type>(ss.end(), ss.end())
                );
}
