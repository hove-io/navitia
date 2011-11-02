#include "data_structures.h"
#include <boost/foreach.hpp>

namespace webservice {

    RequestMethod parse_method(const std::string & method) {
        if(method == "GET") return GET;
        if(method == "POST") return POST;
        else return UNKNOWN;
    }


    ResponseData::ResponseData(const ResponseData & resp) :
                content_type(resp.content_type),
                response(resp.response.str()),
                status_code(resp.status_code),
                charset(resp.charset)
        {
        }


    ResponseData::ResponseData() : content_type("text/plain"), status_code(200), charset("utf-8"){}

    RequestData::RequestData() : params_are_valid(true) {}

    RequestParameter::RequestParameter() : valid_value(true), used_value(true) {}

    RequestParameter ApiMetadata::convert_parameter(const std::string &key, const std::string &value) const {
        auto api_param = params.find(key);
        webservice::RequestParameter param;
        if(api_param != params.end()){
            switch(api_param->second.type){
            case ApiParameter::STRING:
                param.value = value;
                break;
            case ApiParameter::INT:
                try{
                    param.value = boost::lexical_cast<int>(value);
                }catch(boost::bad_lexical_cast){ param.valid_value = false;}
                break;
            case ApiParameter::DOUBLE:
                try{
                    param.value = boost::lexical_cast<double>(value);
                }catch(boost::bad_lexical_cast){ param.valid_value = false;}
                break;
            case ApiParameter::DATE:
                try{
                    param.value = boost::gregorian::from_undelimited_string(value);
                }catch(...){ param.valid_value = false;}
                break;
            case ApiParameter::TIME:
                try{
                // L'heure est au format 945 et on veut récupérer le nombre de secondes depuis minuit
                    int time = boost::lexical_cast<int>(value);
                    param.value = (time / 100) * 60 + (time % 60);
                }catch(boost::bad_lexical_cast){param.valid_value = false;}
                break;
            case ApiParameter::DATETIME:
                try{
                    param.value = boost::posix_time::from_iso_string(value);
                }catch(...){ param.valid_value = false;}
                break;
            case ApiParameter::BOOLEAN:
                if(value == "1") param.value = true;
                else if(value == "0") param.value = false;
                else param.valid_value = false;
                break;
            }
            if(api_param->second.accepted_values.size() > 0){
                if(std::find(api_param->second.accepted_values.begin(), api_param->second.accepted_values.end(), param.value) 
                        == api_param->second.accepted_values.end()){
                    param.valid_value = false;

                }
            }
        }else{
            param.value = value;
            param.used_value = false;
        }
        return param;
    }

    void ApiMetadata::check_manadatory_parameters(RequestData& request) {
        std::pair<std::string, ApiParameter> p;
        BOOST_FOREACH(p, this->params){
            // On a un paramètre obligatoire et qui n'est pas renseigné
            if(p.second.mandatory && request.params.find(p.first) == request.params.end()){
                request.missing_params.push_back(p.first);
                request.params_are_valid = false;
            }
        }
    }

    void ApiMetadata::parse_parameters(RequestData& request){
        std::pair<std::string, std::string> p;
        BOOST_FOREACH(p, request.params){
            webservice::RequestParameter param = this->convert_parameter(p.first, p.second);
            request.parsed_params[p.first] = param;
            request.params_are_valid &= param.valid_value;
        }
    }
}
