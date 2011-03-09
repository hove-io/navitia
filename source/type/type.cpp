#include "type.h"
#include <iostream>
#include <boost/assign.hpp>

bool ValidityPattern::is_valid(int duration){
    if(duration < 0){
        std::cerr << "La date est avant le début de période" << std::endl;
        return false;
    }
    else if(duration > 366){
        std::cerr << "La date dépasse la fin de période" << std::endl;
        return false;
    }
    return true;
}

void ValidityPattern::add(boost::gregorian::date day){
    long duration = (day - beginning_date).days();
    add(duration);
}

void ValidityPattern::add(int duration){
    if(is_valid(duration))
        days[duration] = true;
}

void ValidityPattern::remove(boost::gregorian::date date){
    long duration = (date - beginning_date).days();
    remove(duration);
}

void ValidityPattern::remove(int day){
    if(is_valid(day))
        days[day] = false;
}

static_data * static_data::instance = 0;
static_data * static_data::get() {
    if (instance == 0) {
        instance = new static_data();

        boost::assign::insert(instance->criterias)(Initialization, "Initialization")(AsSoonAsPossible, "AsSoonAsPossible")(LeastInterchange, "LeastInterchange")(LinkTime, "LinkTime")(Debug, "Debug")(WDI,"WDI");
    }
    return instance;
}
