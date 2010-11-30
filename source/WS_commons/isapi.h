#pragma once

#include <HttpExt.h>
#include <boost/function.hpp>
#include "threadpool.h"
#include "data_structures.h"

static int count = 0;
boost::mutex count_mutex;
namespace webservice {
    typedef EXTENSION_CONTROL_BLOCK RequestHandle;

    static boost::function<void(RequestHandle*)> push_request;
    static boost::function<void()> stop_threadpool;
    
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
		request_data.params = handle->lpszQueryString;

        ResponseData resp = w(request_data, data);


        std::stringstream ss;
        ss << "Content-Type: " << resp.content_type << "\r\n\r\n";

		SendHttpHeaders(handle, "200 OK", ss.str().c_str(), FALSE);
        DWORD response_length = resp.response.length();
        
        //char * foo = "foo";
        //response_length = strlen(foo);
        if(!handle->WriteClient(handle->ConnID, (LPVOID)resp.response.c_str(), &response_length, HSE_IO_SYNC)){
            int err = GetLastError(); 
        }
        //BOOST_ASSERT(handle->WriteClient(handle->ConnID, foo, &response_length, HSE_IO_SYNC));
        //BOOST_ASSERT(response_length == resp.response.length());
        handle->ServerSupportFunction(handle->ConnID, HSE_REQ_DONE_WITH_SESSION, NULL, NULL, NULL);
/*
        char *header =
    "HTTP/1.1 200 OK\n\r\n\r"
	"<html>"
      "<head>"
        "<title>test v2</title>"
      "</head>"
      "<body>"
        "<h1>test v2</h1><p>";
	

	DWORD size = static_cast< DWORD >( strlen( header ) );
	handle->WriteClient( handle->ConnID, header, &size, 0 );


    handle->ServerSupportFunction(handle->ConnID, HSE_REQ_DONE_WITH_SESSION, NULL, NULL, NULL);
	*/
    }


}


extern "C"
{
DWORD WINAPI HttpExtensionProc(IN EXTENSION_CONTROL_BLOCK *pECB)
{
	/* Return HSE_STATUS_PENDING to release IIS pool thread without losing connection */
    count_mutex.lock();
    count++;
    count_mutex.unlock();   
     webservice::push_request(pECB);
    
	return HSE_STATUS_PENDING;
}


BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer) {
	
	pVer->dwExtensionVersion = HSE_VERSION;
	strncpy_s( pVer->lpszExtensionDesc, HSE_MAX_EXT_DLL_NAME_LEN, "Hello ISAPI Extension", _TRUNCATE );
	return TRUE;
}

BOOL WINAPI TerminateExtension(DWORD dwFlags) {
    webservice::stop_threadpool();
  return TRUE;
}

}