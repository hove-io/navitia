#pragma once

/** Allows to relationalish queries on any datastructure stored in std::vector
  *
  * We use vocabulary form relationnal databases, however the querying and indexing possibilities
  * are rather poor compared to a real DB
  *
  * The user must define the datastructures he want to use: struct City {std::string name; int population;}
  * and store them in std::vectors
  *
  * There are four operations :
  * — Filter
  * – Join (like an inner join in a DB)
  * – Index to cache a previous query to speed it up
  * – Sort
  * — any combination of thoses
  */

#include <functional>
#include <boost/foreach.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/regex.hpp>
#include <boost/shared_container_iterator.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/view/reverse_view.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/adapted/array.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/array.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/mpl/transform.hpp>


/// In a row of a join, gets the element of type E
template<class E, class T>
E & join_get(T tuple){
    return **(boost::fusion::find<E*>(tuple));
}

/// In a row of a join, gets the element of type E
template<class E, class T>
E & join_get2(T tuple){
    return *(boost::fusion::find<E>(tuple));
}

/** Functor that tests if an attribute is equal to the value passed at construction
  *
  * Example: is_equal<std::string>(&StopPoint::name, "Châtelet");
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

/** Helper function to build an is_equal_t functor
  *
  * Example: auto func = is_equal(&StopPoint::name, "Châtelet");
  */
template<class Attribute, class T>
is_equal_t<Attribute, T> is_equal(Attribute T::*attr, const Attribute & ref){
    return is_equal_t<Attribute, T>(attr, ref);
}


/** Functor that matches an attribute with a regular expression
  *
  * The attribute must be a string
  */
template<class T>
struct matches_t {
    const boost::regex e;
    std::string T::*attr;
    matches_t(std::string T::*attr, const std::string & e) : e(e,boost::regbase::normal | boost::regbase::icase), attr(attr){}
    bool operator()(const T & elt) const {return boost::regex_match(elt.*attr, e );}
};

/** Helper function to build matches_t functor
  *
  * Example: auto func = matches(&StopArea::name, ".*Lyon.*")
  */
template<class T>
matches_t<T> matches(std::string T::*attr, const std::string & e) {
    return matches_t<T>(attr, e);
}



/** Functor that compares if two attributes of two classes are equal
  * Used for joins
  */
template<class T1, class A1, class T2, class A2>
struct attribute_equals_t {
    A1 T1::*attr1;
    A2 T2::*attr2;
    attribute_equals_t(A1 T1::*attr1, A2 T2::*attr2) : attr1(attr1), attr2(attr2) {}
    template<class Tuple>
    bool operator()(const T1 & t1, const Tuple & t2){ return t1.*attr1 == join_get<T2>(t2).*attr2;}
    template<class Tuple1, class Tuple2>
    bool operator()(const Tuple1 & t1, const Tuple2 & t2){return join_get<T1>(t1).*attr1 == join_get<T2>(t2).*attr2;}
};

/** Helper function to use attribute_equals_t
  *
  * Example: attribute_equals(&Stop::city, &City::name)
  */
template<class T1, class A1, class T2, class A2>
attribute_equals_t<T1, A1, T2, A2> attribute_equals(A1 T1::*attr1, A2 T2::*attr2){
    return attribute_equals_t<T1, A1, T2, A2>(attr1, attr2);
}

/** Functor that compares two attributes
  * The attributes must implement operator<
  * Used for sorting
  */
template<class T, class A>
struct attribute_cmp_t {
    A T::*attr;
    attribute_cmp_t(A T::*attr) : attr(attr) {}
    bool operator()(const T & t1, const T & t2){ return t1.*attr <t2.*attr;}
};

/** Helper function to use attribute_cmp_t
  * Example: attribute_cmp(&City::name)
  */
template<class T, class A>
attribute_cmp_t<T,A> attribute_cmp(A T::*attr){return attribute_cmp_t<T,A>(attr);}


/** This class allows is the result of a request
  *
  * It actually is nothing more than a glorified pair of iterators
  * It is built to enable to chain queries (for example first filter, then sort)
  *
  * As no data is stored, it is very lightweight and no iteration is when building the subset
  *
  * When iterating over the subset every operation (joins, filter, sorting...) is repeated.
  * To have better performances, it is possible to create an index from a subset
  */
template<class Iter>
class Subset {
    Iter begin_it;
    Iter end_it;

    /// Functor used for sorting
    template<class Functor>
    struct Sorter{
        Iter begin_it;
        Functor f;

        Sorter(Iter begin_it, Functor f) : begin_it(begin_it), f(f){}

        bool operator()(size_t a, size_t b){
            return f(*(begin_it + a), *(begin_it + b));
        }
    };
public:
    typedef typename Iter::value_type value_type; ///< Type of the data manipulated
    typedef typename Iter::pointer pointer; ///< Pointer type to the data manipulated
    typedef Iter iterator;
    typedef Iter const_iterator;

    /// Creates a subset from a range
    Subset(const Iter & begin_it, const Iter & end_it) :  begin_it(begin_it), end_it(end_it) {}
    iterator begin() {return begin_it;}
    iterator end() {return end_it;}
    const_iterator begin() const {return begin_it;}
    const_iterator end() const {return end_it;}

    /// Returns a new subset filtered according to the functor
    template<class Functor>
    Subset<boost::filter_iterator<Functor, iterator> >
            filter(Functor f) const{
        return Subset<boost::filter_iterator<Functor, iterator> >
                (   boost::make_filter_iterator(f, begin(), end()),
                    boost::make_filter_iterator(f, end(), end())
                    );
    }

    /// Returns a new subset filtered where the attribute matches a regular expression
    Subset<boost::filter_iterator<matches_t<value_type>, iterator> >
            filter_match(std::string value_type::*attr, const std::string & str ) {
        return filter(matches(attr, str));
    }

    /// Returns a new subset where the attribute is equal to a value
    template<class Attribute, class Param>
    Subset<boost::filter_iterator<is_equal_t<Attribute, value_type>, iterator> >
            filter(Attribute value_type::*attr, Param value ) {
        return filter(is_equal<Attribute, value_type>(attr, Param(value)));
    }

    /** Returns a new subset where the rows are sorted according to the functor
      *
      * Implementation is based on boost::permutation_iterator
      * We build an std::vector<int> of the same length and use the functor Sorter to
      * order them according to the functor given by the user
      *
      */
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

/// Helper function tu build a subset from a range
template<class Iter>
Subset<Iter> make_subset(Iter begin, Iter end) {
    return Subset<Iter>(begin, end);
}

/// Helper function tu build a subset from a container
template<class Container>
Subset<typename Container::iterator> make_subset(Container & c) {
    return make_subset(c.begin(), c.end());
}

/// Free function to filter any range by a functor (returns a subset)
template<class Container, class Functor>
Subset<typename boost::filter_iterator<Functor, typename Container::iterator> >
        filter(typename Container::iterator begin, typename Container::iterator end, Functor f) {
    auto subset = make_subset(begin, end);
    return subset.filter(f);
}

/// Free function to filter any container by a functor (returns a subset)
template<class Container, class Functor>
Subset<typename boost::filter_iterator<Functor, typename Container::iterator> > filter(Container & c, Functor f) {
    auto subset = make_subset(c.begin(), c.end());
    return subset.filter(f);
}

/// Free function to filter any container where the attributes of any element is equals to a value (returns a subset)
template<class Container, class Attribute>
Subset<typename boost::filter_iterator<is_equal_t<Attribute, typename Container::value_type>,typename Container::iterator > >
        filter(Container & c, Attribute Container::value_type::*attr, const Attribute & val) {
    auto f = is_equal(attr, val);
    return filter(c, f);
}

/// Free function that sorts a container according to an attribute. The attribute must have an operator< (returns a subset)
template<class Container, class Attribute>
Subset<typename boost::permutation_iterator<typename Container::iterator, typename boost::shared_container_iterator<typename std::vector<ptrdiff_t> > > >
      order(Container & c, Attribute Container::value_type::*attr){
    auto tmp = make_subset(c);
    return tmp.order(attribute_cmp(attr));
}

/** The Index class iterates over a subset in order to index the positions for a faster access
  *
  * It stores the position a pointer to the first element and the difference to the following elements
  * Therefor the memory consumption is roughly an int for each indexed element
  *
  * Warning : if you want to serialize an index, the underlying datastructure MUST be an stl::vector
  * (or a plain C array, but who uses that?)
  *
  * Template parameters:
  * – Type: type we manipulate
  * – IndexType: conta
  */
template<class Type, class IndexContainer, class GetIntFunctor>
class BaseIndex {
public:
    typedef Type value_type; ///< Type of the element we want to index
    typedef value_type & result_type; ///< Type returned when we access to an element

public:
    /// Pointer to the first element
    value_type* begin_it;

    /// Stores where the elements are
    IndexContainer indexes;

    /** This functor takes the start pointer, the difference and returns a reference to the element
      * It is used to get back the data
      */
    struct Transformer{
        /// Functor that given an index returns the int
        mutable GetIntFunctor get_int;
        value_type* begin;
        Transformer(value_type * begin){this->begin = begin;}
        value_type & operator()(typename IndexContainer::value_type idx) const {
            return *(begin + get_int(idx));
        }
    };

    /** Same transformer, only that it returns a constant reference */
    struct ConstTransformer{
        /// Functor that given an index returns the int
        mutable GetIntFunctor get_int;
        value_type* begin;
        ConstTransformer(value_type * begin){this->begin = begin;}
        const value_type & operator()(typename IndexContainer::value_type idx) const {
            return *(begin + get_int(idx));
        }
    };

public:

    /** We use boost::transform iterator to create an iterator that will automatically
      * get the reference of the element from the start pointer and the difference
      */
    typedef typename boost::transform_iterator<Transformer, typename IndexContainer::const_iterator, value_type> iterator;
    typedef typename boost::transform_iterator<ConstTransformer, typename IndexContainer::const_iterator> const_iterator;

    iterator begin(){return iterator(indexes.begin(), Transformer(begin_it));}
    iterator end(){return iterator(indexes.end(), Transformer(begin_it));}

    const_iterator begin() const {return const_iterator(indexes.begin(), ConstTransformer(begin_it));}
    const_iterator end() const {return const_iterator(indexes.end(), ConstTransformer(begin_it));}

    /// Used for serialization
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & begin_it;
    }
};

/// Functor Identity for an int
struct Identity {int operator()(int i){return i;}};

/// Basic index that will give back the data in the same order as the subset
template<class Type>
class Index : public BaseIndex<Type, std::vector<int>, Identity >{
public:
    typedef Type value_type;
    Index(){}

    /** Builds the index from a range
      * It will iterate on the whole range
      */
    template<class Iterator>
    Index(Iterator begin, Iterator end) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(value_type & element, std::make_pair(begin, end)) {
            // We keep the difference between two pointers
            this->indexes.push_back(&element - this->begin_it);
        }
    }

    /// Gets the n-th element
    value_type & operator[](int idx){
        return *(this->begin_it + idx);
    }
};

/// Functor that given a pair, returns the second element
struct GetSecond{template<class Pair> typename Pair::second_type operator()(Pair & p){return p.second;}};

/** Index to access any element by a string */
template<class Type>
class StringIndex: public BaseIndex<Type, std::map<std::string, int>, GetSecond >{
public:
    typedef Type value_type;
    StringIndex() {}

    /** Builds the index from a range
      * It will iterate on the whole range
      */
    template<class Iterator>
    StringIndex(Iterator begin, Iterator end, typename std::string value_type::* attr) {
        this->begin_it = &(*begin);
        BOOST_FOREACH(value_type & element, std::make_pair(begin, end)) {
            this->indexes[element.*attr] = &element - this->begin_it;//on stock la différence entre les deux pointeurs
        }
    }

    /// Get an element with the key
    value_type & operator[](const std::string & key){
        std::map<std::string, int>::iterator it = this->indexes.find(key);
        if(it == this->indexes.end()){
            throw std::out_of_range("Key not found");
        }
        return *(this->begin_it + it->second);
    }
};

/** An index that saves a Join
  * As a join is made of n tables, we store
  * – a boost::fusion::vector<t1*,t2*,...> to the pointer of the first element of each table
  * – a vector of boost::array<int, n> for the offset of each element to the first element
  */
template<class Type>
class JoinIndex {
    typedef Type value_type;
    typedef typename boost::fusion::result_of::size<value_type>::type value_size;
    typedef typename boost::fusion::result_of::as_vector<value_type>::type begin_it_t;
    typedef typename boost::array<int, value_size::value> idx_tuple;
    typedef typename boost::fusion::result_of::as_vector<idx_tuple>::type idx_vector;

    begin_it_t begin_it;
    std::vector<idx_vector> indexes;

    /** Functor that stores a boost::fusion::vector of n pointer (of arbitrary types)
      * It takes a boost::array of n ints and
      * It returns a boost::fusion::vector of references to n elements that represent one row of the join
      */
    struct Transformer{
        typedef typename boost::mpl::transform<value_type, typename boost::reference_wrapper<typename boost::remove_pointer<typename boost::mpl::_1> > >::type result_type;
        begin_it_t begin;

        /** Given a pointer and an idx, it returns the a wrapped reference to the element
          * It is used by a boost::fusion::transform an applied to boost::fusion::vector
          */
        struct get_element{
            template <typename Sig> struct result;

            /** We need to get rid of any decorator (const, &, *)
              * The return type is a boost::ref wrapper around the data
              */
            template <typename T1, typename T2>
            struct result<get_element(T1, T2)> {typedef typename boost::reference_wrapper<
                                                typename boost::remove_pointer<
                                                typename boost::remove_const<
                                                typename boost::remove_reference<T1>::type
                                                >::type
                                                >::type
                                                > type; };
            template<class T1, class T2>
            typename result<get_element(T1,T2)>::type operator()(T1 begin, T2 position) const {return boost::ref(*(begin + position));}
        };
        
        Transformer(begin_it_t begin){
            this->begin = begin;
        }

        /// We apply get_element to every element of the boost::fusion::vector to get the row of a join
        result_type operator()(idx_vector diff) const {
            return boost::fusion::transform(begin, diff, get_element());
        }

        template<class Archive> void serialize(Archive & ar, const unsigned int ) {
            ar & indexes & begin_it;
        }
        
    };

    public:
    typedef typename boost::transform_iterator<Transformer, typename std::vector<idx_vector>::iterator> iterator;
    typedef typename boost::transform_iterator<Transformer, typename std::vector<idx_vector>::const_iterator> const_iterator;

    /** Functor that returns and int that is equal to the offset of two pointers */
    struct ptr_diff{
        typedef int result_type;
        template<class T1>
        int operator()(const T1 * begin, const T1 * current) const {return current - begin;}
    };

    JoinIndex(){}

    /** Builds a Join index from a range.
      * It iterates over all rows of the join
      */
    template<class Iterator>
    JoinIndex(Iterator begin, Iterator end) {
        this->begin_it = *begin;
        BOOST_FOREACH(const value_type & element, std::make_pair(begin, end)) {
            indexes.push_back(boost::fusion::transform(begin_it, element, ptr_diff()));
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

/// Helper function that builds an index of a container
template<class Type>
Index<typename Type::value_type> make_index(const Type & t) {
    return Index<typename Type::value_type>(t.begin(), t.end());
}

/// Helper function that builds a stringIndex of a container on one attribute
template<class Type>
StringIndex<typename Type::value_type> make_string_index(Type & t, typename std::string Type::value_type::*attr) {
    return StringIndex<typename Type::value_type>(t.begin(), t.end(), attr);
}

/// Helper function that builds a JoinIndex of a join
template<class Type>
JoinIndex<typename Type::value_type> make_join_index(Type & t){
    return JoinIndex<typename Type::value_type>(t.begin(), t.end());
}

/** An iterator on a join
  * To allow to chain multiple joins, it is templated by Head:Tail
  * However, when dereferenced, a row is flatten into a single boost::fusion::vector instead of this recursive structure
  *
  * It is just a lazy iterator: there is no iteration on the data until iterates on it
  */
template<class Head, class Tail, class Functor>
class join_iterator{
public:
    /// Output type, a boost::fusion::vector that is the concatenation of Tail and Head
    typedef typename boost::fusion::result_of::as_vector< typename boost::fusion::result_of::push_front<typename Tail::value_type, typename Head::value_type*>::type>::type value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category; 
    typename Head::iterator begin1, current1, end1;

    Tail begin2, current2, end2;
    Functor f;

    /** We build a join_iterator from two ranges and a functor
      * The functor's operator() must return true if two lines shall be joined
      */
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

    /// The dereference returns a boost::fusion::vector of pointers
    value_type operator*() const {
        return boost::fusion::push_front(*current2, &(*current1));
    }

    bool operator==(const join_iterator & other) const { return other.current1 == current1 && other.current2 == current2;}

    /* To get next value: to ways to get out of the function
     * 1) we reached the end (current1==end1 && current2==end2)
     * 2) we got a couple validating the functor
     */
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

    join_iterator<Head, Tail, Functor>& operator++(){
        increment();
        return *this;
    }
};


template<class Head>
class join_iterator<Head, boost::fusion::nil, boost::fusion::nil> {
    typename Head::iterator begin, current, end;

public:
    typedef typename boost::fusion::vector<typename Head::value_type*> value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category;
    join_iterator(typename Head::iterator begin, typename Head::iterator end):
            begin(begin), current(begin), end(end)//, f(f)
    {}

    value_type operator*() const{
        typename Head::value_type & result = *current;
        return value_type(&result);
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

template<class T>
struct remove_subset{
    typedef T type;
};

template<class T>
struct remove_subset< Subset<T> >{
    typedef T type;
};

template<class Container1, class Container2, class Functor>
Subset<join_iterator<Container1, join_iterator<Container2,typename boost::fusion::nil,typename boost::fusion::nil>, Functor> >
        make_join(Container1 & c1, Container2 & c2, const Functor & f) {
    typedef join_iterator<Container2,typename boost::fusion::nil,typename boost::fusion::nil> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Container1, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Container1, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}


template<class Container1, class Container2, class Functor>
Subset<join_iterator<Container1, join_iterator<Subset<Container2>,typename boost::fusion::nil,typename boost::fusion::nil>, Functor> >
        make_join(Container1 & c1, Subset<Container2> c2, const Functor & f) {
    typedef join_iterator<Subset<Container2>,typename boost::fusion::nil,typename boost::fusion::nil> join_c2_t;
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
Subset<join_iterator<Subset<Container1>, join_iterator<Container2,typename boost::fusion::nil,typename boost::fusion::nil>, Functor> >
        make_join(Subset<Container1> c1, Container2 & c2, const Functor & f) {
    typedef join_iterator<Container2,typename boost::fusion::nil,typename boost::fusion::nil> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}


template<class Container1, class Container2, class Functor>
Subset<join_iterator<Subset<Container1>, join_iterator<Subset<Container2>,typename boost::fusion::nil,typename boost::fusion::nil>, Functor> >
        make_join(Subset<Container1> c1, Subset<Container2> c2, const Functor & f) {
    typedef join_iterator<Subset<Container2>,typename boost::fusion::nil,typename boost::fusion::nil> join_c2_t;
    auto tmp = make_join(c2);
    return make_subset(
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.begin(), c1.end(), tmp.begin(), tmp.end(), f),
            join_iterator<Subset<Container1>, join_c2_t, Functor>(c1.end(), c1.end(), tmp.end(), tmp.end(), f)
            );
}

template<class Container>
Subset<join_iterator<Container,typename boost::fusion::nil,typename boost::fusion::nil> >
        make_join(Container & c) {
    return make_subset (
            join_iterator<Container,typename boost::fusion::nil,typename boost::fusion::nil>(c.begin(), c.end()),
            join_iterator<Container,typename boost::fusion::nil,typename boost::fusion::nil>(c.end(), c.end())
            );
}


template<class T1, class T2>
class indexed_join_iterator{
public:
    typedef typename boost::fusion::vector<typename T1::pointer, typename T2::pointer> value_type;
    typedef size_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::forward_iterator_tag iterator_category;
    typename T1::iterator current1, end1;
    typename T2::iterator begin2;
    int T1::value_type::*attr;

    indexed_join_iterator(typename T1::iterator c1_begin, typename T2::iterator c2_begin, int T1::value_type::*attr) :
                    current1(c1_begin), begin2(c2_begin), attr(attr) {
    }

    /// The dereference returns a boost::fusion::vector of pointers
    value_type operator*() const {
        int idx = (*current1).*attr;
        return value_type(&(*current1), &(*(begin2 + idx)));
    }

    bool operator==(const indexed_join_iterator & other) const { return other.current1 == current1;}

    void increment(){
        ++current1;
    }

    indexed_join_iterator<T1, T2>& operator++(){
        increment();
        return *this;
    }
};

template<class T1, class T2>
Subset<indexed_join_iterator<T1, T2> > make_indexed_join(T1 & t1, T2 & t2, int T1::value_type::*attr) {
    return make_subset(
            indexed_join_iterator<T1, T2>(t1.begin(), t2.begin(), attr),
            indexed_join_iterator<T1, T2>(t1.end(), t2.end(), attr)
            );
}
