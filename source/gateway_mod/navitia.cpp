#include "navitia.h"
#include <curl/curl.h>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <log4cplus/logger.h>
#include <boost/format.hpp>


Response::Response() : code(0){}
Response::Response(int code) : code(code){}
Response::Response(const std::string& body, int code) : code(code), body(body){}

Response Navitia::query(const std::string& request){
    std::stringstream ss;
    curlpp::Easy curl_request;
    
    curl_request.setOpt(curlpp::options::WriteStream(&ss));
    std::string req = this->url + request;
    curl_request.setOpt(curlpp::options::Url(req));
    curl_request.setOpt(curlpp::options::ConnectTimeout(5));
    curl_request.setOpt(curlpp::options::NoSignal(1));

    try{
        curl_request.perform();
        long response_code = curlpp::infos::ResponseCode::get(curl_request);
        if(response_code < 200 || response_code >= 300){
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_WARN(logger, boost::format("réponse %d depuis %s") % response_code % req);
        }
            Response response(ss.str(), response_code);
            response.content_type = curlpp::infos::ContentType::get(curl_request);
            return response;
    }catch(curlpp::LibcurlRuntimeError& e){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, boost::format("exception depuis %s -- Curl Reason: %s -- Curl Code: %d") % req % e.what() % e.whatCode());

        if(e.whatCode() == CURLE_OPERATION_TIMEDOUT){
            throw RequestException(RequestException::TIMEOUT);
        }
        throw RequestException();

    }catch(curlpp::RuntimeError& e){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, e.what() + std::string(" - ") + req);
        throw RequestException();
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

void Navitia::on_error(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    this->enable = false;
    this->last_errors_at = time(NULL);
    this->nb_errors++;
    this->reactivate_at = this->last_errors_at + this->nb_errors * desactivation_time;
}

void Navitia::reactivate(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    this->enable = true;
    this->nb_errors--;
    this->next_decrement = time(NULL) + 60;

}
void Navitia::decrement_error(){
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    if(nb_errors > 0){
        this->nb_errors--;
        this->next_decrement = time(NULL) + 60;
    }
}
