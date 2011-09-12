#include "navitia.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <log4cplus/logger.h>
#include <boost/format.hpp>

std::pair<int, std::string> Navitia::query(const std::string& request){
    std::stringstream ss;
    std::stringstream response;
    curlpp::Easy curl_request;
    
    curl_request.setOpt(curlpp::options::WriteStream(&ss));
    std::string req = this->url + request;
    curl_request.setOpt(curlpp::options::Url(req));
    curl_request.setOpt(curlpp::options::ConnectTimeout(5));
    curl_request.setOpt(curlpp::options::NoSignal(1));

    try{
        curl_request.perform();
        long response_code = curlpp::infos::ResponseCode::get(curl_request);
        if(response_code >= 200 && response_code < 300){
            //tous va bien, on renvoie le flux
            return std::make_pair(response_code, ss.str());
        }else{
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_WARN(logger, boost::format("réponse %d depuis %s") % response_code % req);
            throw response_code;
        }
    }catch(curlpp::RuntimeError& e){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, e.what());
        throw 500L;
    }
}


void Navitia::load(){
    this->query("/load");
}

void Navitia::use(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread--;
    current_thread++;
    last_request_at = time(NULL);
}

void Navitia::release(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread++;
    current_thread--;

}
