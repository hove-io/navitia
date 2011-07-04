#pragma once

#include <HttpExt.h>
namespace webservice {typedef EXTENSION_CONTROL_BLOCK RequestHandle; /**< Handle de la requête*/}
#include "data_structures.h"
#include "configuration.h"

namespace webservice {
     /// Envoie les en-têtes
    BOOL SendHttpHeaders(EXTENSION_CONTROL_BLOCK *pECB, LPCSTR pszStatus, LPCSTR pszHeaders, BOOL fKeepConnection){
        HSE_SEND_HEADER_EX_INFO header_ex_info;

        BOOL success;

        header_ex_info.pszStatus = pszStatus;
        header_ex_info.pszHeader = pszHeaders;
        header_ex_info.cchStatus = strlen(pszStatus);
        header_ex_info.cchHeader = strlen(pszHeaders);
        header_ex_info.fKeepConn = fKeepConnection;

        success = pECB->ServerSupportFunction(pECB->ConnID,	HSE_REQ_SEND_RESPONSE_HEADER_EX, &header_ex_info,	NULL,	NULL);

        return success;
    }

    /// Gère la requête (lecture des paramètres)
    template<class Worker, class Data> void request_parser(RequestHandle* handle, Worker & w, Data & data){
        RequestData request_data;
        DWORD totalBytes = handle->cbTotalBytes;

        if( totalBytes > 0) {
            char * tmp_str = new char[totalBytes + 1];
            BOOST_ASSERT(handle->ReadClient(handle->ConnID, tmp_str, &totalBytes));
            BOOST_ASSERT(totalBytes == handle->cbTotalBytes);
            request_data.data = tmp_str;
            delete tmp_str;
        }
        request_data.path = handle->lpszPathInfo;
        request_data.raw_params = handle->lpszQueryString;
        request_data.method = parse_method(handle->lpszMethod);

        ResponseData resp = w(request_data, data);


        std::stringstream ss;
        ss << "Content-Type: " << resp.content_type << "; charset=" << resp.charset <<"\r\n\r\n";

        SendHttpHeaders(handle, "200 OK", ss.str().c_str(), FALSE);
        DWORD response_length = resp.response.str().length();

        if(!handle->WriteClient(handle->ConnID, (LPVOID)resp.response.str().c_str(), &response_length, HSE_IO_SYNC)){
            int err = GetLastError(); 
        }
        handle->ServerSupportFunction(handle->ConnID, HSE_REQ_DONE_WITH_SESSION, NULL, NULL, NULL);
    }


}


extern "C"
{
    DWORD WINAPI HttpExtensionProc(IN EXTENSION_CONTROL_BLOCK *pECB)
    {
        webservice::push_request(pECB);
        /* Return HSE_STATUS_PENDING to release IIS pool thread without losing connection */
        return HSE_STATUS_PENDING;
    }


    BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer) {

        pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );
        strncpy( pVer->lpszExtensionDesc, "Hello ISAPI Extension", HSE_MAX_EXT_DLL_NAME_LEN);
        return TRUE;
    }

    BOOL WINAPI TerminateExtension(DWORD dwFlags) {
        webservice::stop_threadpool();
        return TRUE;
    }
	

}


#define MAKE_WEBSERVICE(Data, Worker) static webservice::ThreadPool<Data, Worker> * tp;\
	extern "C"{\
	BOOL WINAPI DllMain(__in  HINSTANCE hinstDLL, __in  DWORD fdwReason, __in  LPVOID lpvReserved){\
		if(fdwReason == DLL_PROCESS_ATTACH){hinstance = hinstDLL;\
	    Configuration * conf = Configuration::get();\
		tp = new webservice::ThreadPool<Data, Worker>();}\
		return TRUE;}}