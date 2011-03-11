#include "type.h"
#include <iostream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

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

        boost::assign::insert(instance->criterias)
                (cInitialization, "Initialization")
                (cAsSoonAsPossible, "AsSoonAsPossible")
                (cLeastInterchange, "LeastInterchange")
                (cLinkTime, "LinkTime")
                (cDebug, "Debug")
                (cWDI,"WDI");

        boost::assign::insert(instance->point_types)
                (ptCity,"City")
                (ptSite,"Site")
                (ptAddress,"Address")
                (ptStopArea,"StopArea")
                (ptAlias,"Alias")
                (ptUndefined,"Undefined")
                (ptSeparator,"Separator");

        boost::assign::push_back(instance->true_strings)("true")("+")("1");

        using std::locale; using boost::posix_time::time_input_facet;
        boost::assign::push_back(instance->date_locales)
                ( locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")) )
                ( locale(locale::classic(), new time_input_facet("%Y/%m/%d %H:%M:%S")) )
                ( locale(locale::classic(), new time_input_facet("%d/%m/%Y %H:%M:%S")) )
                ( locale(locale::classic(), new time_input_facet("%d.%m.%Y %H:%M:%S")) )
                ( locale(locale::classic(), new time_input_facet("%d-%m-%Y %H:%M:%S")) )
                ( locale(locale::classic(), new time_input_facet("%Y-%m-%d")) );
    }
    return instance;
}


PointType static_data::getpointTypeByCaption(const std::string & strPointType){
    return instance->point_types.right.at(strPointType);
}

Criteria static_data::getCriteriaByCaption(const std::string & strCriteria){
    return instance->criterias.right.at(strCriteria);
}

bool static_data::strToBool(const std::string &strValue){
    return std::find(instance->true_strings.begin(), instance->true_strings.end(), strValue) != instance->true_strings.end();
}

boost::posix_time::ptime static_data::parse_date_time(const std::string& s) {
    boost::posix_time::ptime pt;
    boost::posix_time::ptime invalid_date;
    static_data * inst = static_data::get();
    BOOST_FOREACH(const std::locale & locale, inst->date_locales){
        std::istringstream is(s);
        is.imbue(locale);
        is >> pt;
        if(pt != boost::posix_time::ptime())
            break;
    }
    return pt;
}
