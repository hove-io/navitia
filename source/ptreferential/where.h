#pragma once
#undef TRUE
/** This file provides a way to build functors in a SQL-inspired approach */
namespace navitia{
    namespace ptref{
/// What is the type of the current leaf
enum Node_e {AND, OR, LEAF, NOT, TRUE};


/// This is the base class of a clause node
/// it has two sons right and left that are combined with a logical operation
template<class T>
struct BaseWhere {
    /// Default constructor : it's a clause restricting nothing
    BaseWhere() : current_node(TRUE) {}

    BaseWhere(Node_e current_node) : current_node(current_node) {}

    /// Constructor combining two clauses


    BaseWhere( boost::shared_ptr<BaseWhere> left, boost::shared_ptr<BaseWhere> right, Node_e nt) :
        current_node(nt), left(left), right(right)
    {}

    /// What kind de node are we ?
    Node_e current_node;

    /// Left node; is the current node if we're a LEAF
    boost::shared_ptr< BaseWhere > left;

    /// Right node. Nil if we are at a LEAF
    boost::shared_ptr< BaseWhere > right;

    /// Tests the clause holds for the given instance
    virtual bool eval(const T &) const {
        return true;
    }

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

    virtual ~BaseWhere(){}
};

template<class T>
struct WhereWrapper {
    boost::shared_ptr< BaseWhere<T> > ptr;

    WhereWrapper(const WhereWrapper & ww) : ptr(ww.ptr){}
    WhereWrapper(BaseWhere<T> * w) : ptr(w) {}

    bool operator()(const T & object) const {return (*ptr)(object);}
};

/// Possible Operators
enum Operator_e{EQ, NEQ, LT, GT, LEQ, GEQ, HAVING, AROUND};

/// A specialized clause working on a member and comparing it to a value
template<class T, class T2>
struct Where : public BaseWhere<T> {
    typedef BaseWhere<T> Base;

    /// The pointer to the member we're interested in
    T2 T::* ptr;

    /// The operator to apply if it's a LEAF node
    Operator_e op;

    /// The value we to apply the operator with
    T2 value;

    /// Tests the clause holds for the given instance
    bool eval(const T & element) const {
        switch(op) {
            case EQ: return element.*ptr == value; break;
            case NEQ: return element.*ptr != value; break;
            case LT: return element.*ptr < value; break;
            case GT: return element.*ptr > value; break;
            case LEQ: return element.*ptr <= value; break;
            case GEQ: return element.*ptr >= value; break;
            default : return true;
        }
        return false;
    }

    /// Constructor with a clause
    /// It is overladed to handle the case we give a string to cast it to the right type
    Where(T2 T::* ptr, Operator_e op, const std::string & value) : Base(LEAF), ptr(ptr), op(op), value(boost::lexical_cast<T2>(value)) {}
    Where(T2 T::* ptr, Operator_e op, int value) : Base(LEAF), ptr(ptr), op(op), value(value) {}

    ~Where(){}
};

/// Comfort wrapper to create a where clause
template<class T, class M, class V>
WhereWrapper<T> WHERE(M T::* ptr, Operator_e op, V value) {
    return WhereWrapper<T>(new Where<T,M>(ptr, op, value));
}

/// Logical AND between two clauses
template<class T>
WhereWrapper<T>operator&&(const WhereWrapper<T> & left, const WhereWrapper<T> & right){
    return WhereWrapper<T>(new BaseWhere<T>(left.ptr, right.ptr, AND));
}

/// Logical OR between two clauses
template<class T>
WhereWrapper< BaseWhere<T> > operator||(const WhereWrapper<BaseWhere<T> > & left, const WhereWrapper<BaseWhere<T> > & right){
    return WhereWrapper<T>(new BaseWhere<T>(left, right, OR));
}

}}// navitia::ptref
