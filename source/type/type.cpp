#include "type.h"
#include <iostream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include "data.h"

namespace navitia { namespace type {

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

std::vector<idx_t> Country::get(Type_e type, const Data &)  const {
    std::vector<idx_t> result;
    switch(type) {
    case eDistrict: return district_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> City::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eStopArea: return stop_area_list; break;
    case eDepartment: result.push_back(department_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopArea::get(Type_e type, const Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case eStopPoint:
        BOOST_FOREACH(StopPoint sp, data.stop_points){
            if (sp.stop_area_idx == idx)
                result.push_back(sp.idx);
        }
        break;
    case eCity: result.push_back(city_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Network::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eLine: return line_list; break;
    default: break;
    }
    return result;
}


std::vector<idx_t> Company::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eLine: return line_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> ModeType::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eLine: return line_list; break;
    case eMode: return mode_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Mode::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eModeType: result.push_back(mode_type_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Line::get(Type_e type, const Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case eModeType: result.push_back(mode_type_idx); break;
    case eMode: return mode_list; break;
    case eCompany: return company_list; break;
    case eNetwork: result.push_back(network_idx); break;
    case eRoute: BOOST_FOREACH(Route route, data.routes) {
            if(route.line_idx == idx)
                result.push_back(route.idx);
        }
        break;
    default: break;
    }
    return result;
}


std::vector<idx_t> Route::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eLine: result.push_back(line_idx); break;
    case eModeType: result.push_back(mode_type_idx); break;
    case eRoutePoint: return route_point_list; break;
    case eVehicleJourney: return vehicle_journey_list; break;
    default: break;
    }
    return result;
}


std::vector<idx_t> VehicleJourney::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eRoute: result.push_back(route_idx); break;
    case eCompany: result.push_back(company_idx); break;
    case eMode: result.push_back(mode_idx); break;
    case eVehicle: result.push_back(vehicle_idx); break;
    case eValidityPattern: result.push_back(validity_pattern_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> RoutePoint::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eRoute: result.push_back(route_idx); break;
    case eStopPoint: result.push_back(stop_point_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopPoint::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eStopArea: result.push_back(stop_area_idx); break;
    case eCity: result.push_back(city_idx); break;
    case eMode: result.push_back(mode_idx); break;
    case eNetwork: result.push_back(network_idx); break;
    default: break;
    }
    return result;
}


std::vector<idx_t> StopTime::get(Type_e type, const Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case eVehicle: result.push_back(vehicle_journey_idx); break;
    case eStopPoint: result.push_back(stop_point_idx); break;
    default: break;
    }
    return result;
}

}} //namespace navitia::type
