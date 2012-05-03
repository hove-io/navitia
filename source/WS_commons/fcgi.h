#pragma once
#include <configuration.h>
#include <fcgiapp.h>
namespace webservice {typedef FCGX_Request RequestHandle; /**< Handle de la requête*/}

#include <csignal>
#include "data_structures.h"
#include "type/pb_utils.h"
#include <readline/readline.h>
#include <readline/history.h>
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
        if(FCGX_GetParam("SCRIPT_FILENAME", handle->envp) != NULL)
                request_data.path = FCGX_GetParam("SCRIPT_FILENAME", handle->envp);
        else if(FCGX_GetParam("SCRIPT_NAME", handle->envp) != NULL)
                request_data.path = FCGX_GetParam("SCRIPT_NAME", handle->envp);

        if(FCGX_GetParam("QUERY_STRING", handle->envp) != NULL)
            request_data.raw_params = FCGX_GetParam("QUERY_STRING", handle->envp);
        else
            request_data.raw_params = "(FCGI) FATAL ERROR: UNKNOWN QUERY_STRING";

        if(FCGX_GetParam("REQUEST_METHOD", handle->envp) != NULL)
            request_data.method = parse_method(FCGX_GetParam("REQUEST_METHOD", handle->envp));
        else
            request_data.method = UNKNOWN;

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
    /// Return false si on jamais utilisé de fcgi
    bool run_fcgi(){
        signal(SIGINT, handle_signal);
        signal(SIGTERM, handle_signal);
        //signal(SIGABRT, handle_signal);
 
        FCGX_Init();
        int rc;
        bool accepted_once = false;

        while(true)
        {
            FCGX_Request * request = new FCGX_Request();
            FCGX_InitRequest(request, 0, 0);
            rc = FCGX_Accept_r(request);
            if(rc<0)
                return accepted_once;
            else
                accepted_once = true;
            webservice::push_request(request);
        }
        return accepted_once;
    }

    template<class Data, class Worker>
    void run_cli(int argc, char** argv){
        Data d;
        Worker w(d);       

        if(argc == 1) {
            static char *line_read = (char *)NULL;
            for(;;){
                if (line_read)
                  {
                    free (line_read);
                    line_read = (char *)NULL;
                  }

                /* Get a line from the user. */
                line_read = readline ("NAViTiA> ");

                /* If the line has any text in it,
                   save it on the history. */
                if (line_read && *line_read)
                {
                    if( strcmp(line_read, "exit") == 0 || strcmp(line_read, "quit") == 0)
                    {
                        std::cout << "\n Bye! See you soon!" << std::endl;
                        exit(0);
                    }
                    add_history (line_read);
                    std::cout << w.run_query(line_read, d);
                }

            }
        }
        else if (argc == 2){
            std::cout << w.run_query(argv[1], d);
        }
        else {
            std::cout << "Il faut exactement zéro ou un paramètre" << std::endl;
        }
    }
}


#define MAKE_WEBSERVICE(Data, Worker) int main(int argc, char** argv){\
    Configuration * conf = Configuration::get();\
    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );\
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));\
    char buf[256];\
    if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");\
    webservice::ThreadPool<Data, Worker> tp;\
    if(!webservice::run_fcgi()) webservice::run_cli<Data, Worker>(argc, argv);\
    tp.stop();\
    return 0;\
}
