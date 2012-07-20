#include "backtrace.h"
#include <boost/foreach.hpp>
#include <boost/units/detail/utility.hpp>
#include <boost/algorithm/string.hpp>


BackTrace::BackTrace(int nbtrace) : nb_trace(nbtrace){

    buffer = new ivoid_t[nb_trace];
    nb_trace_returned = backtrace(buffer, nb_trace);
    // Poistion 0 : toujours la fonction BackTrace
    // Position 1 : toujours la fonction PTRefException
    if (nb_trace_returned > 2){
        char **strings;
        strings = backtrace_symbols(buffer, nb_trace_returned);
        if (strings != NULL) {
             for (int j = 2; j < nb_trace_returned; j++){
                 const std::string functionname(function_name(strings[j]));
                 if (! functionname.empty()){
                    trace_list.push_back(functionname);
                 }
             }
        }
        free(strings);
    }
}

BackTrace::~BackTrace(){
    delete[] buffer;
}

std::string BackTrace::get_BackTrace() const{
    std::string my_msg("");
    BOOST_FOREACH(std::string msg, trace_list) {
        my_msg += "\n" + msg;
    }
    return my_msg;
}
/// retourne le nom de la fonction
std::string BackTrace::function_name(const std::string& my_str){
/* la chaine my_str est type :
 ./georef_test(_ZN5boost9unit_test9ut_detail16callback0_impl_tINS1_6unusedEPFvvEE6invokeEv+0x22) [0x804c866]
*/
    std::vector< std::string > module;
    std::vector< std::string > function;
    std::string to_return;

    boost::algorithm::split(module, my_str, boost::algorithm::is_any_of("("));
    if (module.size() > 1){
        boost::algorithm::split(function, module.at(1), boost::algorithm::is_any_of("+"));
        if (function.size() > 0){
            to_return = function.at(0);
            if (! to_return.empty()){
                to_return = boost::units::detail::demangle(to_return.c_str());
                int pos1 = to_return.find("unable to demangle specified symbol");
                if (pos1 > 0)
                    to_return = "";
            }
        }
    }
    return to_return;
}


