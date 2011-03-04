#pragma once
#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../type/type.h"
#include <rapidxml.hpp>
#include <boost\format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include "boost/regex.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/iostreams/device/file.hpp> 
#include <boost/iostreams/stream.hpp> 
#include <istream>



std::pair<std::string,std::string> initFileParams(){
	//Utiliser  boost:
	char buf[2048];
	DWORD filePath = GetModuleFileName(NULL, buf, 2048);
	std::string::size_type posSlash = std::string(buf).find_last_of( "\\/" );
	std::string::size_type posDot = std::string(buf).find_last_of( "." );
	std::string application_name = std::string(buf).substr(posSlash + 1, posDot - (posSlash + 1));
	std::string application_path = std::string(buf).substr( 0, posSlash) + "\\";
	return std::make_pair(application_name, application_path);
}