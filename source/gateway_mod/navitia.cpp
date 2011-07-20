#include "navitia.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

std::pair<int, std::string> Navitia::query(const std::string& request){
    std::stringstream ss;
    std::stringstream response;
    curlpp::Easy curl_request;
    
    std::cout << (this->url + request) << std::endl;

    curl_request.setOpt(new curlpp::options::WriteStream(&ss));
    curl_request.setOpt(new curlpp::options::Url(this->url + request));

    try{
        curl_request.perform();
        long response_code = curlpp::infos::ResponseCode::get(curl_request);
        if(response_code >= 200 && response_code < 300){
            //tous va bien, on renvoie le flux
            return std::make_pair(response_code, ss.str());
        }else{
            throw response_code;
        }
    }catch(curlpp::RuntimeError e){
        throw e;
    }
}


void Navitia::load(){
    this->query("/load");
}

void Navitia::use(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread--;
    last_request_at = time(NULL);
}

void Navitia::release(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    unused_thread++;

}
