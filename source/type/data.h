#pragma once
#include "type.h"


class Data{

    int i;
    public:
    Container<ValidityPattern> validity_patterns;

    Container<Line> lines;

    Container<Route> routes;

    Container<VehicleJourney> vehicle_journeys;

    Container<StopPoint> stop_points;

    Container<StopArea> stop_areas;

    std::vector<StopTime> stop_times;

    Index1ToN<StopArea, StopPoint> stoppoint_of_stoparea;

    public:
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & validity_patterns & lines & stop_points & stop_areas & stop_times & routes& vehicle_journeys;
    }


    void build_index();


    template<class RequestedType>
    std::vector<RequestedType*> find(std::string RequestedType::* attribute, const char * str){
        return find(attribute, std::string(str));
    }

    template<class RequestedType, class AttributeType>
    std::vector<RequestedType*> find(AttributeType RequestedType::* attribute, AttributeType str){
        std::vector<RequestedType *> result;
        BOOST_FOREACH(RequestedType & item, get<RequestedType>().items){
            if(item.*attribute == str){
                result.push_back(&item);
            }
        }
        return result;
//        return find(attribute, std::string(str));
    }

template<class Type> Container<Type> & get();

/*    template<class AttributeType>
    std::vector<StopArea*> find(AttributeType StopArea::* attribute, const AttributeType & value){
        return std::vector<StopArea*>();
    }

    template<class AttributeType>
    std::vector<StopPoint*> find(AttributeType StopPoint::* attribute, const AttributeType & value){
        return std::vector<StopPoint*>();
    }
*/

};




//template<class Type> Container<Type> & Data::get(){throw "wtf?!";}
