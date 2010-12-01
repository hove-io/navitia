#pragma once

#include <fcgiapp.h>
namespace webservice {typedef FCGX_Request RequestHandle;};

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
        request_data.params = FCGX_GetParam("QUERY_STRING", handle->envp);
        request_data.method = parse_method(FCGX_GetParam("REQUEST_METHOD", handle->envp));

        ResponseData resp = w(request_data, data);

        std::stringstream ss;
        ss << "Status: " << resp.status_code << "\r\n"
                << "Content-Type: " << resp.content_type << "; charset=" << resp.charset <<"\r\n\r\n"
                << resp.response;

        FCGX_FPrintF(handle->out, ss.str().c_str());
        FCGX_Finish_r(handle);
        delete handle;
    }

    /// Lance la boucle de traitement fastcgi
    void run_fcgi(){
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



