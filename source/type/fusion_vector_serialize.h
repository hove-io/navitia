#include <boost/fusion/include/vector10.hpp>
#include <boost/fusion/container/vector/vector10.hpp>
#include <boost/fusion/include/vector10.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/for_each.hpp>

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
        // General situation
        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9, typename Archive>
        void serialize(Archive & ar, vector<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }

        // Specific work arround for Visual Studio
        template <class T0, typename Archive>
        void serialize(Archive & ar, vector1<T0> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, typename Archive>
        void serialize(Archive & ar, vector2<T0,T1> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2, typename Archive>
        void serialize(Archive & ar, vector3<T0,T1,T2> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3, typename Archive>
        void serialize(Archive & ar, vector4<T0,T1,T2,T3> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4, typename Archive>
        void serialize(Archive & ar, vector5<T0,T1,T2,T3,T4> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4,class T5, typename Archive>
        void serialize(Archive & ar, vector6<T0,T1,T2,T3,T4,T5> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6, typename Archive>
        void serialize(Archive & ar, vector7<T0,T1,T2,T3,T4,T5,T6> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6,class T7, typename Archive>
        void serialize(Archive & ar, vector8<T0,T1,T2,T3,T4,T5,T6,T7> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6,class T7,class T8, typename Archive>
        void serialize(Archive & ar, vector9<T0,T1,T2,T3,T4,T5,T6,T7,T8> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
        template <class T0,class T1, class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9, typename Archive>
        void serialize(Archive & ar, vector10<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9> & vec, unsigned int) {
            for_each(vec, serializer<Archive>(ar));
        }
    }
}