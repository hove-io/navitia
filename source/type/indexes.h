/** We want to simmulate a relationnal database Indexes
 *
 * Every "table" is an immutable std::vector<Data> where Data can be any class.
 * All the members are a "column"
 * The data must be in an std::vector
 */

namespace navitia { namespace type {

template<class T>
bool true_functor(const T&) {return true;}

template<class T>
struct Index {
    /// Pointer to the begining of the data structures
    T * begin_ptr;

    /// Contains all the offsets so we can get back all our elements
    std::vector<idx_t> offsets;

    /// Initialises the data having two iterators
    template<class Iterator>
    void initialize(Iterator begin, Iterator end){
        offsets.reserve(end - begin);
        initialize(begin, end, &true_functor<typename Iterator::value_type>);
    }
    
    /// Initialises the data having two iterators and a Functor to filter every thing
    template<class Iterator, class Filter>
    void initialize(Iterator begin, Iterator end, const Filter & f){
        begin_ptr = &(*begin);
        while(begin != end) {
            if(f(*begin)) {
                offsets.push_back(&(*begin) - begin_ptr);
            }
            begin++;
        }
    }


    /// Builds the index from a vector
    template<class Data>
    Index(std::vector<Data> & vect) {
        initialize(vect.begin(), vect.end());
    }

    /// Builds the index from a vector and filters accordingly to the functor
    template<class Data, class Functor>
    Index(std::vector<Data> & vect, const Functor & f) {
        initialize(vect.begin(), vect.end(), f);
    }

    /// Returns the number of lines
    size_t size() const {
        return offsets.size();
    }


    /** This functor takes the start pointer, the difference and returns a reference to the element
      * It is used to get back the data
      */
    template<class Type>
    struct Transformer{
        Transformer() {}
        typedef T result_type;
        T * begin_ptr;
        Transformer(T* begin_ptr) : begin_ptr(begin_ptr){}
        Type operator()(idx_t offset) const {
            return *(begin_ptr + offset);
        }
    };

    typedef typename boost::transform_iterator<Transformer<T&>, typename std::vector<idx_t>::const_iterator> iterator;
    typedef typename boost::transform_iterator<Transformer<const T&>, typename std::vector<idx_t>::const_iterator> const_iterator;
    iterator begin() {
        return iterator(offsets.begin(), Transformer<T&>(begin_ptr));
    }

    iterator end() {
        return iterator(offsets.end(), Transformer<T&>(begin_ptr));
    }

    const_iterator begin() const {
        return const_iterator(offsets.begin(), Transformer<const T&>(begin_ptr));
    }

    const_iterator end() const {
        return const_iterator(offsets.end(), Transformer<const T&>(begin_ptr));
    }

    std::vector<idx_t> get_offsets() const{
        return offsets;
    }
};


}} //namespace navitia::type
