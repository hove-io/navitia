#include "configuration.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#ifdef WIN32
#include "windows.h"
HINSTANCE hinstance = NULL;
#endif


Configuration * Configuration::get() {
    if (instance == 0) {
        instance = new Configuration();
#ifdef WIN32
        char buf[2048];
        DWORD filePath = GetModuleFileName(::hinstance, buf, 2048);
        std::string::size_type posSlash = std::string(buf).find_last_of( "\\/" );
        std::string::size_type posDot = std::string(buf).find_last_of( "." );
        instance->strings["application"] = std::string(buf).substr(posSlash + 1, posDot - (posSlash + 1));
        instance->strings["path"] = std::string(buf).substr( 0, posSlash) + "\\";
#endif
    }
    return instance;
}

bool Configuration::is_instanciated(){
    return instance != 0;
}

void Configuration::load_ini(const std::string & filename){
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(filename, pt);

    for(auto section : pt) {
        for(auto key_val : pt.get_child(section.first)) {
            std::string key = key_val.first;
            std::string value = key_val.second.data();
            instance->ini[section.first][key] = value;
        }
    }
}

bool Configuration::has_section(const std::string &section_name) {
    return instance->ini.find(section_name) != instance->ini.end();
}

std::string Configuration::get_string(const std::string & key){
    mutex.lock_shared();
    std::string ret = strings.at(key);
    mutex.unlock_shared();
    return ret;
}

int Configuration::get_int(const std::string & key){
    mutex.lock_shared();
    int ret = ints[key];
    mutex.unlock_shared();
    return ret;
}

void Configuration::set_string(const std::string & key, const std::string & value){
    mutex.lock();
    strings[key] = value;
    mutex.unlock();
}

void Configuration::set_int(const std::string & key, int value){
    mutex.lock();
    ints[key] = value;
    mutex.unlock();
}


Configuration * Configuration::instance = 0;
