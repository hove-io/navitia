#pragma once
#include <configuration.h>
#include <fcgiapp.h>
namespace webservice {typedef FCGX_Request RequestHandle; /**< Handle de la requête*/}

#include <csignal>
#include "data_structures.h"

namespace webservice {
    /// Gère la requête (lecture des paramètres)
    template<class Worker, class Data> void request_parser(RequestHandle* handle, Worker & w, Data & data){
        int len = 0;
        RequestData request_data;
        char *contentLength = FCGX_GetParam("CONTENT_LENGTH", handle->envp);
        if (contentLength != NULL)
            len = strtol(contentLength, NULL, 10);

        if(len > 0) {
            char * tmp_str = new char[len + 1];
            FCGX_GetStr(tmp_str, len, handle->in);
            request_data.data = tmp_str;
            delete tmp_str;
        }
        request_data.path = FCGX_GetParam("SCRIPT_FILENAME", handle->envp);
        request_data.raw_params = FCGX_GetParam("QUERY_STRING", handle->envp);
        request_data.method = parse_method(FCGX_GetParam("REQUEST_METHOD", handle->envp));

        ResponseData resp = w(request_data, data);

        std::stringstream ss;
        ss << "Status: " << resp.status_code << "\r\n"
                << "Content-Type: " << resp.content_type << "; charset=" << resp.charset <<"\r\n\r\n"
                << resp.response.str();


        FCGX_PutStr(ss.str().c_str(), ss.str().size(), handle->out);
        FCGX_Finish_r(handle);
        delete handle;
    }

    /** Gère les signaux pour un arrêt propre */
    void handle_signal(int){
        stop_threadpool();
        exit(0);
    }


    /// Lance la boucle de traitement fastcgi
    void run_fcgi(){
        signal(SIGINT, handle_signal);
        signal(SIGTERM, handle_signal);
        signal(SIGABRT, handle_signal);
 
        FCGX_Init();
        int rc;

        while(true)
        {
            FCGX_Request * request = new FCGX_Request();
            FCGX_InitRequest(request, 0, 0);
            rc = FCGX_Accept_r(request);
            if(rc<0)
                break;
            webservice::push_request(request);
        }
    }

}


#define MAKE_WEBSERVICE(Data, Worker) int main(int, char** argv){\
    Configuration * conf = Configuration::get();\
    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );\
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));\
    char buf[256];\
    if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");\
    webservice::ThreadPool<Data, Worker> tp;\
    webservice::run_fcgi();\
    return 0;\
}
