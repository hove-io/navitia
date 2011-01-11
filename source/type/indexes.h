#include "type.h"
#include <boost/iterator/transform_iterator.hpp>

/** Indexe pour une relation 1-N
  *
  * Par exemple on veut obtenir tous les StopPoints d'une StopArea
  * StopPoint possède un pointeur vers le StopArea auquel il appartient, mais cet indexe permet d'avoir la relation
  * dans l'autre sens
  */
template<class From, class Target>
class Index1ToN{
    /// Pointeur vers la structure de données cible (ex. StopPoint)
    Container<Target> * targets;

    /// Pointeur vers la structure de données d'origine (ex. StopArea)
    Container<From> * froms;

    /// Pour chaque StopArea, on stoque les index des StopPoints
    std::vector<std::set<int> > items;

    public:

    /// Constructeur par défaut
    Index1ToN(){}

    /** Constructeur à partir des deux structures de données.
      *
      * Il faut préciser quelle est la clef externe qui permet de retrouver le From étant donné le Target
      * Ex: stoppoint_of_stoparea.create(stop_areas, stop_points, &StopPoint::stop_area_idx)
      */
    Index1ToN(Container<From> & from, Container<Target> & target,  int Target::* foreign_key){
        create(from, target, foreign_key);
    }

    /** Construit l'indexe */
    void create(Container<From> & from, Container<Target> & target,  int Target::* foreign_key){
        targets = &target;
        froms = &from;
        items.resize(from.size());
        for(int i=0; i<target.size(); i++){
            unsigned int from_id = target[i].*foreign_key;
            if(from_id > items.size()){
                continue;
            }
            items[from_id].insert(i);
        }
    }

    /** Retourne tous les Targets (ex. StopPoints) étant donné un index de From (ex. StopArea) */
    std::vector<Target*> get(int idx){
        std::vector<Target*> result;
        BOOST_FOREACH(int i, items[idx]) {
            result.push_back(&(*targets)[i]);
        }
        return result;
    }

    /** Retourne tous les Targets (ex. StopPoints) étant donné un code externe de From (ex. StopArea) */
    std::vector<Target*> get(const std::string & external_code){
        return get(froms->get_idx(external_code));
    }

    /** Fonction interne pour sérialiser (aka. binariser) */
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & targets & froms & items;
    }


};

/** Index qui trie les éléments selon un attribut
  *
  * Par exemple les stop areas trié par nom
  */
template<class Type, class Attribute>
class SortedIndex{
    /// Structure de données dont on veut un tri différent
    Container<Type> * items;

    /// Indexes vers items dans l'ordre souhaité
    std::vector<int> indexes;

    /// Functor permettant de trier les données
    struct Sorter{
        Attribute Type::*key;
        Container<Type> * items;

        Sorter(Container<Type> * items, Attribute Type::*key){
            this->items = items;
            this->key = key;
        }

        bool operator()(int a, int b){
            return (*items)[a].*key < (*items)[b].*key;
        }
    };

    public:
    /// Constructeur par défaut, ne fait rien
    SortedIndex(){};

    /// Crée un index sur le conteneur trié par l'attribut key
    SortedIndex(Container<Type> & from, Attribute Type::*key){
        create(from, key);
    }

    /// Construit l'index à proprement parler
    void create(Container<Type> & from, Attribute Type::*key){
        items = &from;
        indexes.resize(from.size());
        for (int i = 0;i < from.size(); i++) {
            indexes[i] = i;
        }
        std::sort(indexes.begin(), indexes.end(), Sorter(items, key));
    }

    /// Retourne le idx-ième élément dans selon l'ordre de trié souhaité dans cet indexe
    Type & get(int idx) {
        return (*items)[indexes[idx]];
    }

    /** Fonction interne pour sérialiser (aka. binariser) */
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & items;
    }
};

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

 template<class Type>
 Index<Type>
 make_index(typename Type::iterator begin, typename Type::iterator end) {
    return Index<Type>(begin, end);
 }

 template<class Type>
 Index<Type> make_index(const Type & t) {
    return Index<Type>(t.begin(), t.end());
 }



