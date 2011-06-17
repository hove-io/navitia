/** We want to simmulate a relationnal database Indexes
 *
 * Every "table" is an immutable std::vector<Data> where Data can be any class.
 * All the members are a "column"
 * The data must be in an std::vector
 */

#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/array.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/fusion/container/vector/detail/as_vector.hpp>

namespace navitia { namespace type {


namespace f = boost::fusion;
namespace m = boost::mpl;

template<class T>
bool true2(const T&) {return true;}

/// TypeVector is a boost::fusion vector of multiple data types
template<class TypeVector>
struct Index2 {
    /// A boost::vector of pointers of data types
    typedef typename boost::mpl::transform<TypeVector, typename boost::add_pointer<typename boost::mpl::_1> >::type TypePtrVector;
    typedef typename boost::mpl::transform<
        typename boost::mpl::transform<TypeVector, typename boost::add_const<typename boost::mpl::_1> >::type,
        typename boost::add_pointer<typename boost::mpl::_1>
    >::type ConstPtrVector;


    /// The number of types we consider
    const static int NbTypes = boost::fusion::result_of::size<TypeVector>::type::value;

    /// Type of the Array containing the offset of every element
    typedef typename boost::array<uint32_t, NbTypes> offset_t;

    /// NbTypes pointers to the begining of the data structures
    TypePtrVector begin_ptr;

    /// Contains all the offsets so we can get back all our elements
    std::vector<offset_t> offsets;

    int nb_types(){return NbTypes;}

    /// Initialises the data having two iterators
    template<class Iterator>
    void initialize(Iterator begin, Iterator end){
        offsets.reserve(end - begin);
        initialize(begin, end, &true2<typename Iterator::value_type>);
    }
    
    /// Initialises the data having two iterators and a Functor to filter every thing
    template<class Iterator, class Filter>
    void initialize(Iterator begin, Iterator end, const Filter & f){
        typename Iterator::pointer first_elt = &(*begin);
        begin_ptr = TypePtrVector(first_elt);
        while(begin != end) {
            if(f(*begin)) {
                offset_t offset;
                offset[0] = &(*begin) - first_elt;
                offsets.push_back(offset); 
            }
            begin++;
        }
    }


    /// Builds the index from a vector
    template<class Data>
    Index2(std::vector<Data> & vect) {
        initialize(vect.begin(), vect.end());
    }

    /// Builds the index from a vector and filters accordingly to the functor
    template<class Data, class Functor>
    Index2(std::vector<Data> & vect, const Functor & f) {
        initialize(vect.begin(), vect.end(), f);
    }

    /// Returns the number of lines
    size_t size() const {
        return offsets.size();
    }


    /** This functor takes the start pointer, the difference and returns a reference to the element
      * It is used to get back the data
      */
    template<class PtrVector>
    struct Transformer{
        template<int N>
        typename boost::disable_if_c<N == 0>::type
        copy(const TypePtrVector & t1, PtrVector & t2, const offset_t & offset) const {
            boost::fusion::at_c<N>(t2) = f::at_c<N>(t1) + offset[N];
            copy<N-1>(t1, t2, offset);
        }

        template<int N>
        typename boost::enable_if_c<N == 0>::type
        copy(const TypePtrVector & t1, PtrVector & t2, const offset_t & offset) const {
            boost::fusion::at_c<N>(t2) = f::at_c<0>(t1) + offset[0];
        }

        Transformer() {}

        typedef PtrVector result_type;
        TypePtrVector begin_ptr;
        Transformer(const TypePtrVector & begin_ptr) : begin_ptr(begin_ptr){}
        PtrVector operator()(const offset_t & offset) const {
            PtrVector ret;
            copy<NbTypes-1>(begin_ptr, ret, offset);
            return ret;
        }
    };

    typedef typename boost::transform_iterator<Transformer<TypePtrVector>, typename std::vector<offset_t>::const_iterator> iterator;
    typedef typename boost::transform_iterator<Transformer<ConstPtrVector>, typename std::vector<offset_t>::const_iterator> const_iterator;
    iterator begin() {
        return iterator(offsets.begin(), Transformer<TypePtrVector>(begin_ptr));
    }

    iterator end() {
        return iterator(offsets.end(), Transformer<TypePtrVector>(begin_ptr));
    }

    const_iterator begin() const {
        return const_iterator(offsets.begin(), Transformer<ConstPtrVector>(begin_ptr));
    }

    const_iterator end() const {
        return const_iterator(offsets.end(), Transformer<ConstPtrVector>(begin_ptr));
    }

    std::vector<offset_t> get_offsets() const{
        return offsets;
    }
};


}} //namespace navitia::type
