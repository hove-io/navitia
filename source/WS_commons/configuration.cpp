#include "configuration.h"


Configuration * Configuration::get() {
    if (instance == 0) {
        instance = new Configuration();
#ifdef WIN32
        char buf[2048];
        DWORD filePath = GetModuleFileName(NULL, buf, 2048);
        std::string::size_type posSlash = std::string(buf).find_last_of( "\\/" );
        std::string::size_type posDot = std::string(buf).find_last_of( "." );
        this->strings["application"] = std::string(buf).substr(posSlash + 1, posDot - (posSlash + 1));
        this->strings["path"] = std::string(buf).substr( 0, posSlash) + "\\";
#endif
    }
    return instance;
}

Configuration * Configuration::instance = 0;
