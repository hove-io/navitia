/** We want to simmulate a relationnal database
 *
 * Every "table" is an immutable std::vector<Data> where Data can be any class.
 * All the members are a "row"
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
        initialize(begin, end, &true2<typename Iterator::value_type>);

    }
    
    /// Initialises the data having two iterators and a Functor to filter every thing
    template<class Iterator, class Filter>
    void initialize(Iterator begin, Iterator end, const Filter & f){
        offsets.reserve(end - begin);
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
};


/// functor that always returns true
template<class T>
struct True {
    bool operator()(const T&) const {return true;}
};
/// Possible Operators
enum Operator_e{EQ, NEQ, LT, GT, LEQ, GEQ};

/// What is the type of the current leaf
enum Node_e {AND, OR, LEAF, NOT, TRUE};

/// Models the WHERE clause as a tree to build a complete syntax of AND, OR...
template<class T>
struct Where{
    /// What kind de node are we ?
    Node_e current_node;

    /// Left node; is the current node if we're a LEAF
    boost::shared_ptr< Where<T> > left;

    /// Right node. Nil if we are at a LEAF
    boost::shared_ptr< Where<T> > right;

    /// The pointer to the member we're interested in
    /// The is one possible instance and we'll use it depending on current_type
    int T::* iptr;
    double T::*dptr;
    std::string * sptr;

    /// The operator to apply if it's a LEAF node
    Operator_e op;

    /// The value we to apply the operator with
    int value;

    /// Possible types we manage
    enum Type_e{INT, DOUBLE, STRING};

    /// What type is the current clause applied to
    Type_e current_type;

    /// Tests the clause holds for the given instance
    bool eval(const T & element) const {
        switch(op) {
            case EQ: return element.*iptr == value; break;
            case NEQ: return element.*iptr != value; break;
            case LT: return element.*iptr < value; break;
            case GT: return element.*iptr > value; break;
            case LEQ: return element.*iptr <= value; break;
            case GEQ: return element.*iptr >= value; break;
        }
        return false;
    }

    /// Default constructor : it restricts nothing
    Where() : current_node(TRUE) {}

    /// Constructor with a clause
    /// It is overladed to match the type we wants
    Where(int T::* ptr, Operator_e op, const std::string & value) : current_node(LEAF), iptr(ptr), op(op), value(boost::lexical_cast<int>(value)), current_type(INT) {}
    Where(int T::* ptr, Operator_e op, int value) : current_node(LEAF), iptr(ptr), op(op), value(value), current_type(INT) {}
    Where(double T::* ptr, Operator_e op, const std::string & value) : current_node(LEAF), dptr(ptr), op(op), value(boost::lexical_cast<double>(value)), current_type(DOUBLE) {}
    Where(double T::* ptr, Operator_e op, double value) : current_node(LEAF), dptr(ptr), op(op), value(value), current_type(DOUBLE) {}
    Where(std::string T::* ptr, Operator_e op, const std::string & value) : current_node(LEAF), sptr(ptr), op(op), value(value), current_type(STRING) {}

    /// Constructor combining two clauses
    Where(const Where<T> & left, const Where<T> & right, Node_e nt) : current_node(nt), left(new Where<T>(left)), right(new Where<T>(right)) {}

    /// Evaluates if the clause is valid 
    bool operator()(const T & element) const {
        switch(current_node) {
            case AND: return (*left)(element) && (*right)(element); break;
            case OR: return (*left)(element) || (*right)(element); break;
            case LEAF: return eval(element); break;
            case NOT: return !eval(element); break;
            case TRUE: return true; break;
        }
        return true;
    }
};

template<class T, class M, class V>
Where<T> WHERE(M T::* ptr, Operator_e op, V value) {
    return Where<T>(ptr, op, value);
}

template<class T>
Where<T> operator&&(const Where<T> & left, const Where<T> & right){
    return Where<T>(left, right, AND);
}

template<class T>
Where<T> operator||(const Where<T> & left, const Where<T> & right){
    return Where<T>(left, right, OR);
}
