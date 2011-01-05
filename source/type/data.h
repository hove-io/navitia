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

    SortedIndex<StopArea, std::string> stop_area_by_name;

    public:
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & validity_patterns & lines & stop_points & stop_areas & stop_times & routes 
            & vehicle_journeys & stoppoint_of_stoparea & stop_area_by_name;
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
    }

    template<class Type> Container<Type> & get();


    void save(const std::string & filename);
    void load(const std::string & filename);
    void save_bin(const std::string & filename);
    void load_bin(const std::string & filename);


};

