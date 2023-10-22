
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//   HTTPClient.c
//
// Abstract: Partially Implements the client side of the HTTP 1.1 Protocol as
//           Defined in RFC 2616,2617
// Platform: Any that supports standard C calls and Berkeley sockets
//
// Author:	Eitan Michaelson
// Version:  1.0
// Date      7/1/2006
// Opens:    HTTP Proxy authentication support (not fully tested).
//           HTTP Keep Alive not impliamented
//           HTTPS not implemented
//           The HTTP counters are not 100% correct (will be fixed)
//
///////////////////////////////////////////////////////////////////////////////

#include "HTTPClient.h"
#include "HTTPClientAuth.h"     // Crypto support (Digest, MD5)
#include "HTTPClientString.h"   // String utilities

#ifndef _WIN32

/**
 * _RCW_ : Added functions.
 */
#ifdef HTTPC_LWIP
#define FCNTL lwip_fcntl
#else
#include	<fcntl.h>
#define FCNTL fcntl
#endif
/**
 * Here to set a socket to nonblocking (or not).
 */
int sktSetNonblocking( int socket , int on_off )
{
        int			flags_orig , flags_new;

        // Get current socket flags; return if error.

        flags_orig = FCNTL( socket , F_GETFL , 0 );
        if ( flags_orig < 0 )
                return -1;

        // Adjust flags.
        if ( on_off != 0 )
                flags_new = flags_orig | O_NONBLOCK;
        else
                flags_new = flags_orig & ~O_NONBLOCK;

        // Set socket to blocking/nonblocking; return if error.
        if ( FCNTL( socket , F_SETFL , flags_new ) < 0 )
        {
                // Set flags back to where they were.
                FCNTL( socket , F_SETFL , flags_orig );
                // And return.
                return -1;
        }
        return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetLocalConnection
// Purpose      : TBD
// Gets         :
// Returns      :
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientSetLocalConnection  (HTTP_SESSION_HANDLE pSession, UINT32 nPort)
{

        return 0;
}
///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientOpenRequest
// Purpose      : Allocate memory for a new HTTP Session
// Gets         : FLAGS
// Returns      : HTTP Session handle
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

HTTP_SESSION_HANDLE  HTTPClientOpenRequest (HTTP_CLIENT_SESSION_FLAGS Flags)
{


        P_HTTP_SESSION pHTTPSession = NULL;         // Handle to the session pointer
        UINT32         nAllocationSize;             // Size of the dynamically allocated buffer

        // Attempt to allocate the buffer

        pHTTPSession = (P_HTTP_SESSION)malloc(ALIGN(sizeof(HTTP_SESSION)));

        // Did we succeed?
        if(!pHTTPSession)
        {
                // Null pointer is returned upon error
                return 0;
        }

        HC_DBG(("Session Ptr(pHTTPSession) lives at %p.", (void*)&pHTTPSession));

        // Reset the allocated space to zeros
        memset(pHTTPSession,0x00,sizeof(HTTP_SESSION));

        // Allocate space for the incoming and outgoing headers
        // Check if the headers buffer is resizable
        if(HTTP_CLIENT_MEMORY_RESIZABLE)
        {
                // Memory is resizable, so use the init defined size or the maximum buffer size (which ever is smaller)
                nAllocationSize = MIN(HTTP_CLIENT_MAX_SEND_RECV_HEADERS,HTTP_CLIENT_INIT_SEND_RECV_HEADERS);

        }
        else
        {
                // Memory is not resizable so simply use the maximum defined size
                nAllocationSize = HTTP_CLIENT_MAX_SEND_RECV_HEADERS;
        }
        // Allocate the headers buffer
        pHTTPSession->HttpHeaders.HeadersBuffer.pParam = (CHAR*)malloc(ALIGN(nAllocationSize));
        // Check the returned pointer
        if(!pHTTPSession->HttpHeaders.HeadersBuffer.pParam)
        {
                // malloc() error, free the containing structure and exit.
                free(pHTTPSession);
                return 0;

        }

        // Reset the headers allocated memory
        memset(pHTTPSession->HttpHeaders.HeadersBuffer.pParam ,0x00,nAllocationSize);
        // Set the buffer length
        pHTTPSession->HttpHeaders.HeadersBuffer.nLength = nAllocationSize;

        // Set default values in the session structure
        HTTPClientSetVerb((UINT32)(unsigned long)pHTTPSession,(HTTP_VERB)HTTP_CLIENT_DEFAULT_VERB);    // Default HTTP verb
        pHTTPSession->HttpUrl.nPort             = HTTP_CLIENT_DEFAULT_PORT;             // Default TCP port
        pHTTPSession->HttpConnection.HttpSocket = HTTP_INVALID_SOCKET;                       // Invalidate the socket
        // Set the outgoing headers pointers
        pHTTPSession->HttpHeaders.HeadersOut.pParam = pHTTPSession->HttpHeaders.HeadersBuffer.pParam;
        // Set our state
        pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_INIT;

        // Save the flags
        pHTTPSession->HttpFlags = Flags;

        // Reset the status
        pHTTPSession->HttpHeadersInfo.nHTTPStatus = 0;
        // Return an allocated session pointer (cast to UINT32 first)
        return (HTTP_SESSION_HANDLE)(unsigned long)pHTTPSession;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientReset
// Purpose      : Reset the HTTP session and make it ready for another call
// Gets         : FLAGS
// Returns      : HTTP Session handle
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32  HTTPClientReset (HTTP_SESSION_HANDLE pSession)
{

        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        return HTTPIntrnSessionReset(pHTTPSession,TRUE);

}

///////////////////////////////////////////////////////////////////////////////
// Function     : HTTPClientCloseRequest
// Purpose      : Closes any active connection and free any used memory
// Gets         : a Session handle
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientCloseRequest (HTTP_SESSION_HANDLE *pSession)
{
        P_HTTP_SESSION pHTTPSession = NULL;
        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)*(pSession);

        if(!pHTTPSession)
        {
                // User passed a bad pointer
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
        // Check for a valid pointer to the HTTP headers
        if(pHTTPSession->HttpHeaders.HeadersBuffer.pParam)
        {
                // Release the used memory
                free(pHTTPSession->HttpHeaders.HeadersBuffer.pParam);
        }
        // Close any active socket connection
        HTTPIntrnConnectionClose(pHTTPSession);
        // free the session structure
        free(pHTTPSession);

        pHTTPSession = 0;   // NULL the pointer
        *(pSession) = 0;

        return HTTP_CLIENT_SUCCESS;
}

#ifdef _HTTP_DEBUGGING_
///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetDebugHook
// Purpose      : Sets the HTTP debug function pointer
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32  HTTPClientSetDebugHook (HTTP_SESSION_HANDLE pSession,
                E_HTTPDebug *pDebug)  // [IN] Function pointer to the the caller debugging function)
{

        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        pHTTPSession->pDebug =  pDebug;
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPClientSetDebugHook",NULL,0,"Debugging hook set, return code is %d",HTTP_CLIENT_SUCCESS);
        }
#endif


        return HTTP_CLIENT_SUCCESS;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetVerb
// Purpose      : Sets the HTTP verb for the outgoing request
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32  HTTPClientSetVerb (HTTP_SESSION_HANDLE pSession,
                HTTP_VERB HttpVerb)  // [IN] Http verb (GET POST HEAD act')
{

        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPClientSetVerb",NULL,0,"Selected Verb is %d",(INT32)HttpVerb);
        }
#endif

        // Cache the verb (as integer) for later use
        pHTTPSession->HttpHeaders.HttpVerb = HttpVerb;

        // Convert the Verb parameter into its equivalent string representation
        switch (HttpVerb)
        {
                case VerbGet:
                        strcpy(pHTTPSession->HttpHeaders.Verb,"GET");
                        break;
                case VerbHead:
                        if(!HTTP_CLIENT_ALLOW_HEAD_VERB)
                        {
                                return HTTP_CLIENT_ERROR_BAD_VERB;
                        }
                        strcpy(pHTTPSession->HttpHeaders.Verb,"HEAD");
                        break;
                case VerbPost:
                        strcpy(pHTTPSession->HttpHeaders.Verb,"POST");
                        break;
                default:
                        // Unknown verb
                        HC_ERR(("No support verb."));
                        return HTTP_CLIENT_ERROR_BAD_VERB;
        };
        HC_DBG(("Selected Verb(0:get 1:head: 2post):%d.",HttpVerb));
        return HTTP_CLIENT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetVerb
// Purpose      : Sets the HTTP authentication schema
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32  HTTPClientSetAuth (HTTP_SESSION_HANDLE pSession,
                HTTP_AUTH_SCHEMA AuthSchema,  // The type of the authentication to perform
                void *pReserved)
{
        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPClientSetAuth",NULL,0,"Selected authentication is %d",(INT32)AuthSchema);
        }
#endif

        switch(AuthSchema)
        {
                case(AuthSchemaBasic):
                        strcpy(pHTTPSession->HttpCredentials.AuthSchemaName,"basic");
                        break;
                case(AuthSchemaDigest):
                        strcpy(pHTTPSession->HttpCredentials.AuthSchemaName,"digest");
                        break;
                case(AuthSchemaKerberos):
                        strcpy(pHTTPSession->HttpCredentials.AuthSchemaName,"negotiate");
                        break;
                default:
                        break;
        };

        if(AuthSchema >= AuthNotSupported)
        {
                return HTTP_CLIENT_ERROR_BAD_AUTH;
        }
        HC_DBG(("Selected auth(1:basic 2:digest): %d ..",AuthSchema));
        pHTTPSession->HttpCredentials.CredAuthSchema =  AuthSchema;
        return HTTP_CLIENT_SUCCESS;
}
#ifdef HTTPC_PROXY
///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetProxy
// Purpose      : Sets all the proxy related parameters
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientSetProxy (HTTP_SESSION_HANDLE pSession,
                CHAR *pProxyHost,        // [IN] Null terminated string containing the host name
                UINT16 nPort,            // [IN] The proxy port number
                CHAR *pUserName,         // [IN] User name for proxy authentication (can be null)
                CHAR *pPassword)         // [IN] User password for proxy authentication (can be null)
{
        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPClientSetProxy",NULL,0,"Proxy host %s,Port %d, Username %s,Password %s",pProxyHost,nPort,pUserName,pPassword);
        }
#endif


        // Cache the user supplied information in the internal session structure
        strncpy(pHTTPSession->HttpProxy.ProxyHost,pProxyHost,HTTP_CLIENT_MAX_PROXY_HOST_LENGTH);
        if(pUserName)
        {
                // Proxy user name (for Proxy server authentication)
                strncpy(pHTTPSession->HttpProxy.ProxtUser,pUserName,HTTP_CLIENT_MAX_USERNAME_LENGTH);
        }
        if(pPassword)
        {
                // Proxy password (for proxy server authentication)
                strncpy(pHTTPSession->HttpProxy.ProxyPassword,pPassword,HTTP_CLIENT_MAX_PASSWORD_LENGTH);
        }
        // Proxy TCP port
        pHTTPSession->HttpProxy.nProxyPort = nPort;

        // Set the Proxy flag in the connection structure
        pHTTPSession->HttpFlags = pHTTPSession->HttpFlags | HTTP_CLIENT_FLAG_USINGPROXY;
        // Set the proxy auyjentication schema
        if(pPassword && pUserName)
        {
                pHTTPSession->HttpProxy.ProxyAuthSchema = HTTP_CLIENT_DEFAULT_PROXY_AUTH;
        }
        return HTTP_CLIENT_SUCCESS;
}
#endif
///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetCredentials
// Purpose      : Sets credentials for the target host
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientSetCredentials (HTTP_SESSION_HANDLE pSession,
                CHAR *pUserName,   // [IN] Null terminated string containing the sessions user name
                CHAR *pPassword)   // [IN] Null terminated string containing the sessions password
{
        UINT32  nLength;
        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        // Get the length of the user name string and see if it's not too long
        nLength = strlen(pUserName);
        if(nLength > HTTP_CLIENT_MAX_USERNAME_LENGTH)
        {
                HC_ERR(("Username length too long."));
                return HTTP_CLIENT_ERROR_LONG_INPUT;
        }

        // Get the length of the password string and see if it's not too long
        nLength = strlen(pPassword);
        if(nLength > HTTP_CLIENT_MAX_PASSWORD_LENGTH)
        {
                HC_ERR(("Passwd length too long."));
                return HTTP_CLIENT_ERROR_LONG_INPUT;
        }

        // Copy them into our internal buffer
        strcpy(pHTTPSession->HttpCredentials.CredUser ,pUserName);
        strcpy(pHTTPSession->HttpCredentials.CredPassword ,pPassword);

        // Set the authentication Boolean flag
        pHTTPSession->HttpHeadersInfo.HaveCredentials = TRUE;
        HC_DBG(("cred (%s:%s)",pUserName,pPassword));
        return HTTP_CLIENT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientAddRequestHeaders
// Purpose      : Add headers to the outgoing request
// Gets         :
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientAddRequestHeaders (HTTP_SESSION_HANDLE pSession,
                CHAR *pHeaderName,      // [IN] The Headers name
                CHAR *pHeaderData,      // [IN] The headers data
                BOOL nInsert)           // [IN] Reserved could be any
{

        UINT32  nRetCode;
        UINT32  nHeaderLength,nDataLength;
        P_HTTP_SESSION pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPClientAddRequestHeaders",NULL,0,"Adding Header %s: %s",pHeaderName,pHeaderData);
        }
#endif

        // Get the elements length
        nHeaderLength = strlen(pHeaderName);
        nDataLength   = strlen(pHeaderData);

        // Call the internal function to add the headers to our session buffer
        nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,pHeaderName,nHeaderLength,pHeaderData,nDataLength);

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSetHeaderExtention
// Purpose      : Add user headers to the outgoing request
// Gets         :
// Last updated : 29/08/2017
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientSetHeaderExtention(P_HTTP_SESSION pHTTPSession, CHAR *Headers, UINT32 Length)
{
        UINT32          nRetCode = 0;
        CHAR            *pHttpHeaders = Headers;
        CHAR            *pHttpHeader = NULL;
        CHAR            *pHttpHeaderValue = NULL;
        CHAR            *pHttpHeadersEnd = pHttpHeaders + Length;
        CHAR            *pPtr = NULL;
        UINT32          nHttpHeader = 0;
        UINT32          nHttpHeaderValue = 0;

        do {
                pHttpHeader = pHttpHeaders;
                if ((pPtr = strstr(pHttpHeaders, ":")) != NULL) {
                        nHttpHeader = pPtr - pHttpHeader;
                } else {
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                pHttpHeaders = pPtr + 1;
                pHttpHeaderValue = pHttpHeaders;
                if ((pPtr = strstr(pHttpHeaders, "&")) != NULL) {
                        nHttpHeaderValue = pPtr - pHttpHeaderValue;
                } else {
                        nHttpHeaderValue = pHttpHeadersEnd - pHttpHeaderValue;
                }
                if (pPtr != NULL)
                        pHttpHeaders = pPtr + 1;
                else
                        pHttpHeaders = NULL;
                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,pHttpHeader,nHttpHeader,
                                                pHttpHeaderValue,nHttpHeaderValue))!= HTTP_CLIENT_SUCCESS) {
                        break;
                }
                if (pHttpHeaders == NULL)
                        break;

        } while(1);

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientSendRequest
// Purpose      : This function builds the request headers, performs a DNS resolution ,
//                opens the connection (if it was not opened yet by a previous request or if it has closed)
//                and sends the request headers.
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientSendRequest (HTTP_SESSION_HANDLE pSession,
                CHAR *pUrl,           //  [IN] The requested URL
                VOID *pData,          //  [IN] Data to post to the server
                UINT32 nDataLength,   //  [IN] Length of posted data
                BOOL TotalLength,     //  [IN]
                UINT32 nTimeout,      //  [IN] Operation timeout
                UINT32 nClientPort)   //  [IN] Client side port 0 for none
{

        UINT32          nRetCode;               // Function call return code
        UINT32          nBytes;                 // Bytes counter (socket operations)
        UINT32          nUrlLength;             // Length of the given Url
        P_HTTP_SESSION  pHTTPSession = NULL;    // Session pointer
        CHAR            ContentLength[32];
        HC_DBG(("url:%s",pUrl));
        do
        {

                // Cast the handle to our internal structure and check the pointers validity
                pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
                if(!pHTTPSession)
                {
                        HC_ERR(("pHTTPSession is NULL..\n"));
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        pHTTPSession->pDebug("HTTPClientSendRequest",NULL,0,"Url: %s",pUrl);
                }
#endif

                // Set the operation timeout counters
                pHTTPSession->HttpCounters.nActionStartTime = HTTPIntrnSessionGetUpTime();
                // 0 makes us use the default defined value
                pHTTPSession->HttpCounters.nActionTimeout = HTTP_TIMEOUT(nTimeout);
                // Store the cliebt port for later usage
                pHTTPSession->HttpConnection.HttpClientPort = nClientPort;

                // Parse the URL
                nUrlLength = strlen(pUrl);
                nRetCode = HTTPIntrnSetURL(pHTTPSession,pUrl,nUrlLength);
                if(nRetCode != HTTP_CLIENT_SUCCESS)
                {
                        HC_ERR(("set url failed...\n"));
                        break;
                }
                HC_DBG(("HttpFlags:%lu",pHTTPSession->HttpFlags));
                // Create the default headers
                // Add the "Host" header. we should handle a special case of port incorporated within the host name.
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_URLANDPORT) != HTTP_CLIENT_FLAG_URLANDPORT)
                {
                        // The case where we don't have the port
                        if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,
                                                        "Host",4,
                                                        pHTTPSession->HttpUrl.UrlHost.pParam,
                                                        pHTTPSession->HttpUrl.UrlHost.nLength)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }
                else
                {
                        // We have the port so use a deferent element
                        if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Host",4,
                                                        pHTTPSession->HttpUrl.UrlHost.pParam,
                                                        (pHTTPSession->HttpUrl.UrlPort.pParam - pHTTPSession->HttpUrl.UrlHost.pParam) -1 )) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }
                // We are in a post request without knowing the total length in advance so return error or use chunking
                if(pHTTPSession->HttpHeaders.HttpVerb == VerbPost && TotalLength == FALSE)
                {
                        // If the user specified the chunked flag
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SEND_CHUNKED) == HTTP_CLIENT_FLAG_SEND_CHUNKED)
                        {
                                // Add the  Transfer-Encoding:  header
                                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Transfer-Encoding",17,"chunked",7))!= HTTP_CLIENT_SUCCESS)
                                {
                                        break;;
                                }
                        }
                        else
                        {
                                // Not a supported operation - unknown length
                                nRetCode = HTTP_CLIENT_ERROR_HEADER_NO_LENGTH;
                                break;
                        }
                }

                // Add the "User-Agent" header
                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"User-Agent",10,HTTP_CLIENT_DEFAULT_AGENT,strlen(HTTP_CLIENT_DEFAULT_AGENT)))!= HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Add the "Keep-Alive" header (if requested by the caller)
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_KEEP_ALIVE) == HTTP_CLIENT_FLAG_KEEP_ALIVE)
                {
#ifdef HTTPC_PROXY
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_USINGPROXY) != HTTP_CLIENT_FLAG_USINGPROXY)
                        {
#endif
                                // No proxy keep alive:
                                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Connection",10,"Keep-Alive",10))!= HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
#ifdef HTTPC_PROXY
                        }
                        else // proxy keep alive
                        {
                                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Proxy-Connection",15,"Keep-Alive",10))!= HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }
#endif
                }


                // If we know the total length in advance (only when posting)
                if(pHTTPSession->HttpHeaders.HttpVerb == VerbPost && TotalLength == TRUE)
                {
                        // set the total content length header
                        pHTTPSession->HttpHeadersInfo.nHTTPPostContentLength = nDataLength; // Store for later usage
                        memset(ContentLength,0,32);
                        IToA(ContentLength,nDataLength); // Convert the buffer length to a string value
                        if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Content-Length",14,ContentLength,strlen(ContentLength)))!= HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }

                }

                // Add the "Cache control" header (if requested by the caller)
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_NO_CACHE) == HTTP_CLIENT_FLAG_NO_CACHE)
                {
                        if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Cache-Control",13,"no-cache",8))!= HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }

                // Add user header
                CHAR          *pHttpHeaders = NULL;
                UINT32         nHttpHeaders = 0;

                if (gHttpcGetHeader.callback != NULL) {

                        pHttpHeaders = gHttpcGetHeader.callback();
                        if (!pHttpHeaders) {
				nRetCode = -1;
                                HC_ERR(("User set header failed,%s",__func__));
                                break;
                        }
                        nHttpHeaders = strlen(pHttpHeaders);

                        if ((nRetCode = HTTPClientSetHeaderExtention(pHTTPSession, pHttpHeaders, nHttpHeaders)) != HTTP_CLIENT_SUCCESS) {
                                HC_ERR(("Add user header failed,%s",__func__));
                                break;
                        }
                }

                // Now we can connect to the remote server and send the leading request followed by the HTTP headers
                // Check for timeout
                if(HTTPIntrnSessionEvalTimeout(pHTTPSession) == TRUE)
                {
                        HC_ERR(("TIMEOUT,%s",__func__));
                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_TIME_OUT;
                        break;
                }
                //  Handle connection close message (reconnect)
                if(pHTTPSession->HttpHeadersInfo.Connection == FALSE)
                {
                        // Gracefully close the connection and set the socket as invalid
                        if(pHTTPSession->HttpConnection.HttpSocket != HTTP_INVALID_SOCKET)
                        {
                                HTTPIntrnConnectionClose(pHTTPSession);
                        }
                        // Connect to the remote server (or proxy)
                        nRetCode = HTTPIntrnConnectionOpen(pHTTPSession);
                        if(nRetCode != HTTP_CLIENT_SUCCESS)
                        {
                                HC_ERR(("connect server failed.."));
                                break;
                        }
                }

                // Send the request along with the rest of the headers
                if(pHTTPSession->HttpCredentials.CredAuthSchema != AuthSchemaNone ||
#ifdef HTTPC_PROXY
                                pHTTPSession->HttpProxy.ProxyAuthSchema != AuthSchemaNone) // If we have to authenticate we should use the HEAD verb
#else
                        0)
#endif
                        {
                                HC_DBG(("send auth request,sch:%d",pHTTPSession->HttpCredentials.CredAuthSchema));
                                if(HTTP_CLIENT_ALLOW_HEAD_VERB) // HEAD should not be ussed if not defined
                                {
                                        if((nRetCode = HTTPIntrnHeadersSend(pHTTPSession,VerbHead)) != HTTP_CLIENT_SUCCESS)
                                        {
                                                HC_ERR(("Send Header failed(verb : HEAD),%d",__LINE__));
                                                break;
                                        }
                                        // Set the state flag
                                        pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HEAD_SENT;
                                }
                                else
                                {

                                        // Simply use the verb that was set by the caller without changing to HEAD
                                        if((nRetCode = HTTPIntrnHeadersSend(pHTTPSession,pHTTPSession->HttpHeaders.HttpVerb)) != HTTP_CLIENT_SUCCESS)
                                        {
                                                HC_ERR(("Send Header failed,%d",__LINE__));
                                                break;
                                        }

                                        // This the case where the caller know the total length to receive in advance
                                        // and he wishes to send the data right away
                                        if(pHTTPSession->HttpHeaders.HttpVerb == VerbPost &&  TotalLength == TRUE)
                                        {

                                                // Send the data
                                                nBytes = nDataLength;
                                                if((nRetCode = HTTPIntrnSend(pHTTPSession,pData,&nBytes)) != HTTP_CLIENT_SUCCESS)
                                                {
                                                        HC_ERR(("Send Header failed,%d",__LINE__));
                                                        break;
                                                }
                                                // Set the session state
                                                pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_POST_SENT;
                                        }

                                }

                                // Retrive and analyze the Headers
                                if((nRetCode = HTTPIntrnHeadersReceive(pHTTPSession,nTimeout)) != HTTP_CLIENT_SUCCESS)
                                {
                                        HC_ERR(("Retrive and analyze failed,%d",__LINE__));
                                        break;
                                }
                                if(pHTTPSession->HttpHeadersInfo.nHTTPStatus != 401 && pHTTPSession->HttpHeadersInfo.nHTTPStatus != 407)
                                {

                                        nRetCode = HTTP_CLIENT_ERROR_AUTH_MISMATCH;
                                        HC_ERR(("AUTH mismatch failed,%d",__LINE__));
                                        break;
                                }
#ifdef HTTPC_AUTH_CLOSE_CONNEC
                                pHTTPSession->HttpHeadersInfo.Connection = FALSE;
#endif
                                // Authenticate
                                if((nRetCode = HTTPIntrnAuthenticate(pHTTPSession)) != HTTP_CLIENT_SUCCESS)
                                {
                                        HC_ERR(("Authenticate failed,%d",__LINE__));
                                        break;
                                }

                        }
                else
                {
                        // No authentication use the verb that was requested by the caller
                        if((nRetCode = HTTPIntrnHeadersSend(pHTTPSession,pHTTPSession->HttpHeaders.HttpVerb)) != HTTP_CLIENT_SUCCESS)
                        {
                                HC_ERR(("Send Header failed,%d",__LINE__));
                                break;
                        }
                }
                // This the case where the caller know the total length to receive in advance
                // and he wishes to send the data right away
                if(pHTTPSession->HttpHeaders.HttpVerb == VerbPost &&  TotalLength == TRUE)
                {
                        // Send the data
                        nBytes = nDataLength;
                        if((nRetCode = HTTPIntrnSend(pHTTPSession,pData,&nBytes)) != HTTP_CLIENT_SUCCESS)
                        {
                                HC_ERR(("Send post data failed,%d",__LINE__));
                                break;
                        }
                        // Set the session state
                        pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_POST_SENT;
                }

        }while(0);



        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientRecvResponse
// Purpose      : Receives the response header on the connection and parses it.
//                Performs any required authentication.
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientRecvResponse (HTTP_SESSION_HANDLE pSession,
                UINT32 nTimeout) // [IN] Timeout for the operation
{

        UINT32          nRetCode;               // Function return code
        P_HTTP_SESSION  pHTTPSession = NULL;    // Session pointer


        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        do
        {
                if((nRetCode = HTTPIntrnHeadersReceive(pHTTPSession, nTimeout)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
        } while(0);

        return nRetCode;

}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientWriteData
// Purpose      : Write data to the remote server
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientWriteData (HTTP_SESSION_HANDLE pSession,
                VOID *pBuffer,
                UINT32 nBufferLength,
                UINT32 nTimeout)
{

        UINT32          nRetCode     = HTTP_CLIENT_SUCCESS;
        UINT32          nBytes;
        CHAR            Chunk[HTTP_CLIENT_MAX_CHUNK_HEADER];

        P_HTTP_SESSION  pHTTPSession = NULL;


        // Cast the handle to our internal structure and check the pointer validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        do
        {
                if(!pHTTPSession)
                {
                        nRetCode =  HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Set the operation timeout counters
                pHTTPSession->HttpCounters.nActionStartTime = HTTPIntrnSessionGetUpTime();
                pHTTPSession->HttpCounters.nActionTimeout = HTTP_TIMEOUT(nTimeout);

                // Did the caller specified chunked sending?
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SEND_CHUNKED) == HTTP_CLIENT_FLAG_SEND_CHUNKED)
                {
                        // Prep the chunk Header and send it
                        memset(Chunk,0x00,HTTP_CLIENT_MAX_CHUNK_HEADER);
                        HTTPStrLToH(Chunk,nBufferLength);
                        strcat(Chunk,HTTP_CLIENT_CRLF);

                        // Send the leading CrLf (only after the first chunk)
                        if(pHTTPSession->HttpCounters.nSentChunks >= 1)
                        {
                                nBytes = 2;;
                                nRetCode = HTTPIntrnSend(pHTTPSession,HTTP_CLIENT_CRLF,&nBytes);
                                if(nRetCode != HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }

                        // Send the chunk header
                        nBytes = strlen(Chunk);
                        nRetCode = HTTPIntrnSend(pHTTPSession,Chunk,&nBytes);
                        if(nRetCode != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }

                // Send the data
                nBytes = nBufferLength;
                nRetCode = HTTPIntrnSend(pHTTPSession,pBuffer,&nBytes);
                if(nRetCode != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // If we are using chunks then..
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SEND_CHUNKED) == HTTP_CLIENT_FLAG_SEND_CHUNKED)
                {
                        // Set the chunks count
                        pHTTPSession->HttpCounters.nSentChunks++;

                        // If it was the last chunk (0) we should re-get the headers from the server reply
                        if(nBufferLength == 0)
                        {
                                // Send the trailing CrLf
                                nBytes = 2;;
                                nRetCode = HTTPIntrnSend(pHTTPSession,HTTP_CLIENT_CRLF,&nBytes);
                                if(nRetCode != HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                                // Get the remote headers (since the last chunk was transmitted we can expect the server to start the reply)
                                if((nRetCode  = HTTPIntrnHeadersReceive(pHTTPSession,nTimeout)) != HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }
                }

        } while(0);

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientReadData
// Purpose      : Read data from the server. Parse out the chunks data.
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientReadData (HTTP_SESSION_HANDLE pSession,
                VOID *pBuffer,           // [IN OUT] a pointer to a buffer that will be filled with the servers response
                UINT32 nBytesToRead,     // [IN]     The size of the buffer (numbers of bytes to read)
                UINT32 nTimeout,         // [IN]     operation timeout in seconds
                UINT32 *nBytesRecived)   // [OUT]    Count of the bytes that ware received in this operation
{

        UINT32          nBytes          = 0;
        UINT32          nRetCode        = 0 ;
        INT32           nProjectedBytes = 0; // Should support negative numbers
#ifdef	HTTP_BUFFER_WITH_NULL_TERM
        CHAR            *pNullPtr;
#endif
        BOOL            EndOfStream = FALSE;

        P_HTTP_SESSION  pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        // If the last verb that was used was HEAD there is no point to get this data (chanses are that we will endup with timeout)
        if(pHTTPSession->HttpHeaders.HttpVerb == VerbHead)
        {
                return HTTP_CLIENT_EOS;

        }

        // Set the operation timeout counters
        pHTTPSession->HttpCounters.nActionStartTime = HTTPIntrnSessionGetUpTime();
        pHTTPSession->HttpCounters.nActionTimeout = HTTP_TIMEOUT(nTimeout);

#ifdef	HTTP_BUFFER_WITH_NULL_TERM
        nBytes               = nBytesToRead - 1; // We will spare 1 byte for the trailing null termination
        *((CHAR*)pBuffer)   = 0;                // Null terminate the user supplied buffer
#else
        nBytes               = nBytesToRead;
#endif
        *(nBytesRecived)    = 0;                // Set the return bytes count to 0

        // We can read the data only if we got valid headers (and not authentication requests for example)
        if((pHTTPSession->HttpState & HTTP_CLIENT_STATE_HEADERS_PARSED) != HTTP_CLIENT_STATE_HEADERS_PARSED)
        {
                return HTTP_CLIENT_ERROR_BAD_STATE;
        }

        // Is it a chunked mode transfer?
        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_CHUNKED) == HTTP_CLIENT_FLAG_CHUNKED)
        {

                // How many bytes left until the next chunk?
                if(pHTTPSession->HttpCounters.nBytesToNextChunk == 0)
                {
                        // Read the chunk header and get its length
                        if(HTTPIntrnGetRemoteChunkLength(pHTTPSession) != HTTP_CLIENT_SUCCESS)
                        {
                                // Could not parse the chunk parameter
                                return HTTP_CLIENT_ERROR_CHUNK;
                        }

                        // 0 Bytes chunk, we should return end of stream
                        if(pHTTPSession->HttpCounters.nRecivedChunkLength == 0)
                        {
                                return HTTP_CLIENT_EOS;
                        }
                }
                // If we are about to read pass the next chunk, reduce the read bytes so we will read
                // non HTML data
                nProjectedBytes = pHTTPSession->HttpCounters.nBytesToNextChunk - nBytes;
                if ( nProjectedBytes <= 0)
                {
                        // Set the correct bytes count we should read
                        nBytes  = pHTTPSession->HttpCounters.nBytesToNextChunk;
                }
        }


        // Do we have the content length?
        if(pHTTPSession->HttpHeadersInfo.nHTTPContentLength > 0)
        {
                // Length of the projected buffer
                nProjectedBytes = pHTTPSession->HttpCounters.nRecivedBodyLength +  nBytes;
                // If we are going to read more then the known content length then..
                if(nProjectedBytes >= (INT32)pHTTPSession->HttpHeadersInfo.nHTTPContentLength)
                {
                        // Reduce the received bytes count to the correct size
                        nBytes = pHTTPSession->HttpHeadersInfo.nHTTPContentLength - pHTTPSession->HttpCounters.nRecivedBodyLength;

                }
        }

        // Receive the data from the socket
        nRetCode = HTTPIntrnRecv(pHTTPSession,(CHAR*)pBuffer,&nBytes,FALSE);

        // Set the return bytes count
        *(nBytesRecived) = nBytes;   // + 1; Fixed 11/9/2005

#ifdef	HTTP_BUFFER_WITH_NULL_TERM
        // Pointer to the end of the buffer
        pNullPtr    = (CHAR*)pBuffer + nBytes ;
        // And null terminate
        *pNullPtr = 0;
#endif
        // Socket read went OK
        if(nRetCode == HTTP_CLIENT_SUCCESS)
        {

#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        pHTTPSession->pDebug("HTTPClientReadData",NULL,0,"Reading %d bytes",nBytes);
                }
#endif
                // Set the HTTP counters
                pHTTPSession->HttpCounters.nRecivedBodyLength += nBytes;
                // If we know the total content length and..
                if(pHTTPSession->HttpHeadersInfo.nHTTPContentLength > 0)
                {
                        // If total received body is equal or greater then the known content length then..
                        if( pHTTPSession->HttpCounters.nRecivedBodyLength >= pHTTPSession->HttpHeadersInfo.nHTTPContentLength)
                        {
                                // Raise a flag to signal end of stream
                                EndOfStream = TRUE;
                        }
                }
                // Is it a chunked mode transfer?
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_CHUNKED) == HTTP_CLIENT_FLAG_CHUNKED)
                {
                        // We are a little closer to the next chunk now
                        pHTTPSession->HttpCounters.nBytesToNextChunk -= nBytes;
                }
                // Is it End of stream?
                if(EndOfStream == TRUE)
                {
                        // So exit

                        return HTTP_CLIENT_EOS;
                }
        }

        return nRetCode ;
}
///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientGetInfo
// Purpose      : Fill the users structure with the session information
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientGetInfo (HTTP_SESSION_HANDLE pSession, HTTP_CLIENT *HTTPClient)
{
        P_HTTP_SESSION  pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
        // Reset the users info structure
        memset(HTTPClient,0x00,sizeof(HTTP_CLIENT));

        HTTPClient->HTTPStatusCode              = pHTTPSession->HttpHeadersInfo.nHTTPStatus;
        HTTPClient->RequestBodyLengthSent       = pHTTPSession->HttpCounters.nSentBodyBytes;
        HTTPClient->ResponseBodyLengthReceived  = pHTTPSession->HttpCounters.nRecivedBodyLength;
        HTTPClient->TotalResponseBodyLength     = pHTTPSession->HttpHeadersInfo.nHTTPContentLength;
        HTTPClient->HttpState                   = pHTTPSession->HttpState;
#ifdef HTTP_GET_REDIRECT_URL
        HTTPClient->RedirectUrl                 = (HTTP_REDIRECT_PARAM *)&(pHTTPSession->HttpHeadersInfo.HttpRedirectURL);
#endif
#ifdef HTTP_GET_HANDLE_FLAGS
        HTTPClient->HttpFlags                   = pHTTPSession->HttpFlags;
#endif
#if defined(HTTP_GET_REDIRECT_URL) && defined(HTTP_GET_HANDLE_FLAGS)
        HC_DBG(("HTTPClient: \nHTTPStatusCode:0x%x, \
                                \nRequestBodyLengthSent:0x%x, \
                                \nResponseBodyLengthReceived:0x%x, \
                                \nTotalResponseBodyLength:0x%x, \
                                \nHttpState:0x%x, \
                                \nHttpFlags:0x%x",
                                (unsigned int)(HTTPClient->HTTPStatusCode),
                                (unsigned int)(HTTPClient->RequestBodyLengthSent),
                                (unsigned int)(HTTPClient->ResponseBodyLengthReceived),
                                (unsigned int)(HTTPClient->TotalResponseBodyLength),
                                (int)(HTTPClient->HttpState),
                                (int)(HTTPClient->HttpFlags)));
#endif

        return HTTP_CLIENT_SUCCESS;


}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientFindFirstHeader
// Purpose      : Initiate the headr searching functions
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientFindFirstHeader (HTTP_SESSION_HANDLE pSession, CHAR *pSearchClue,CHAR *pHeaderBuffer, UINT32 *nLength)
{

        P_HTTP_SESSION  pHTTPSession = NULL;
        UINT32         nClueLength;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        nClueLength = strlen(pSearchClue); // See if we are not to long
        if(nClueLength >= HTTP_CLIENT_MAX_HEADER_SEARCH_CLUE)
        {
                return HTTP_CLIENT_ERROR_HEADER_BIG_CLUE;
        }
        else
        {
                strcpy(pHTTPSession->HttpHeaders.SearchClue,pSearchClue);
                pHTTPSession->HttpHeaders.HeaderSearch.nLength = 0;
                pHTTPSession->HttpHeaders.HeaderSearch.pParam = NULL;
        }

        return HTTP_CLIENT_SUCCESS;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientFindCloseHeader
// Purpose      : Terminate a headers search session
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPClientFindCloseHeader (HTTP_SESSION_HANDLE pSession)
{

        P_HTTP_SESSION  pHTTPSession = NULL;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        pHTTPSession->HttpHeaders.SearchClue[0] = 0;
        pHTTPSession->HttpHeaders.HeaderSearch.nLength = 0;
        pHTTPSession->HttpHeaders.HeaderSearch.pParam = NULL;

        return HTTP_CLIENT_SUCCESS;

}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPClientGetNextHeader
// Purpose      : Terminate a headers search session
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////
UINT32 HTTPClientGetNextHeader (HTTP_SESSION_HANDLE pSession, CHAR *pHeaderBuffer, UINT32 *nLength)
{

        P_HTTP_SESSION  pHTTPSession = NULL;
        UINT32          nOffset = 0;
        UINT32          nRetCode;
        HTTP_PARAM      HttpHeader;
        CHAR            *pPtr;

        // Cast the handle to our internal structure and check the pointers validity
        pHTTPSession = (P_HTTP_SESSION)(unsigned long)pSession;
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
        if(pHTTPSession->HttpHeaders.HeaderSearch.nLength > 0) // We must adjust the search offset since it is not the fierst iteration
        {
                nOffset = pHTTPSession->HttpHeaders.HeaderSearch.pParam - pHTTPSession->HttpHeaders.HeadersIn.pParam;
        }
        // Search for the next header
        nRetCode = HTTPIntrnHeadersFind(pHTTPSession,pHTTPSession->HttpHeaders.SearchClue,&HttpHeader,TRUE,nOffset);

        if(nRetCode == HTTP_CLIENT_SUCCESS)
        {
                if(HttpHeader.nLength > *(nLength)) // Check for sufficiant length
                {
                        *(nLength) = HttpHeader.nLength;
                        pHeaderBuffer[0] = 0; // Reset the users buffer
                        return HTTP_CLIENT_ERROR_NO_MEMORY;
                }

                pPtr = HttpHeader.pParam;
                nOffset = 0;
                if(*pPtr == 0x0d)
                {
                        nOffset++;
                        pPtr++;
                }
                if(*pPtr == 0x0a)
                {
                        nOffset++;
                        pPtr++;
                }

                strncpy(pHeaderBuffer,pPtr,HttpHeader.nLength - nOffset);
                pHeaderBuffer[HttpHeader.nLength - nOffset] = 0;
                *(nLength) = HttpHeader.nLength - nOffset;
                pHTTPSession->HttpHeaders.HeaderSearch.pParam = HttpHeader.pParam + HttpHeader.nLength;
                pHTTPSession->HttpHeaders.HeaderSearch.nLength++;

                return HTTP_CLIENT_SUCCESS;

        }

        return nRetCode;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnSetURL
// Purpose      : Parse the user's URL
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnSetURL (P_HTTP_SESSION pHTTPSession,
                CHAR *pUrl,         // [IN] a null terminated string containing the Url we should retrieve
                UINT32 nUrlLength)  // [IN] The length the Url string
{

        UINT32         nUrlOffset;      // Offset in bytes within the Url string
        HTTP_URL       *pUrlPtr;        // a Pointer to the Url structure (within the global session structure)
        CHAR           *pPtr;           // a Pointer for the Url port (Used in the parsing process)
        CHAR           UrlPort[16];     // a temporary byte array for the Url port conversion operation (string to number)

        // Check for the session pointer validity
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }


        // Get the length of the Url
        nUrlLength = strlen(pUrl);

        // Check if it is not longer than the permitted length
        if((nUrlLength + 1) > HTTP_CLIENT_MAX_URL_LENGTH)
        {
                return HTTP_CLIENT_ERROR_LONG_INPUT;
        }

        // Point the local pointer on the global structure
        pUrlPtr = &pHTTPSession->HttpUrl;

        // Copy to the internal buffer
        memcpy(pHTTPSession->HttpUrl.Url,pUrl,nUrlLength);
        nUrlOffset              = 0;

        // Get the Url base ("http" or "https")
        if(HTTPStrSearch(pUrlPtr->Url,":",nUrlOffset,nUrlLength,&pUrlPtr->UrlBsee) == FALSE)
        {
                return HTTP_CLIENT_ERROR_BAD_URL;
        }
        HC_DBG(("url base:%.*s",(int)pUrlPtr->UrlBsee.nLength,pUrlPtr->UrlBsee.pParam));
        // Increase the offset parameter
        nUrlOffset += pUrlPtr->UrlBsee.nLength;
        // If we can parse the string "HTTPS" we can assume a secured session
        if(HTTPStrInsensitiveCompare(pUrlPtr->UrlBsee.pParam,"https",pUrlPtr->UrlBsee.nLength) == TRUE)
        {
                // Set the secured flags on the session
                pHTTPSession->HttpFlags = pHTTPSession->HttpFlags | HTTP_CLIENT_FLAG_URLHTTPS;
                pHTTPSession->HttpFlags = pHTTPSession->HttpFlags | HTTP_CLIENT_FLAG_SECURE;
                // ToDo: Init TLS (GetProtocol)


#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        pHTTPSession->pDebug("HTTPIntrnSetURL",NULL,0,"SSL Protected Url %s",pUrl);
                }
#endif
        }
        else // it should be "http"
        {
                if(HTTPStrInsensitiveCompare(pUrlPtr->UrlBsee.pParam,"http",pUrlPtr->UrlBsee.nLength) == FALSE)
                {
                        return HTTP_CLIENT_ERROR_BAD_URL; // cOULD NOT DETECT http or https prefix

                }
        }
        // Look for standard Url elements
        if(HTTPStrSearch(pUrlPtr->Url,"://",nUrlOffset,3,0) == FALSE)
        {
                return HTTP_CLIENT_ERROR_BAD_URL; // Could not detect "://"
        }
        // Increase the offset parameter
        nUrlOffset += 3;

        // Get the host name
        if(HTTPStrSearch(pUrlPtr->Url,"/",nUrlOffset,(nUrlLength - nUrlOffset),&pUrlPtr->UrlHost) == FALSE)
        {
                pUrlPtr->Url[nUrlLength] = '/';
                nUrlLength++;
                if(HTTPStrSearch(pUrlPtr->Url,"/",nUrlOffset,(nUrlLength - nUrlOffset),&pUrlPtr->UrlHost) == FALSE)
                {
                        return HTTP_CLIENT_ERROR_BAD_URL;
                }
        }
        HC_DBG(("url host:%.*s",(int)pUrlPtr->UrlHost.nLength,pUrlPtr->UrlHost.pParam));
        nUrlOffset += pUrlPtr->UrlHost.nLength;

        // Do we have the port within the hostname?
        if(HTTPStrSearch(pUrlPtr->Url,":",
                                (nUrlOffset - pUrlPtr->UrlHost.nLength),
                                pUrlPtr->UrlHost.nLength,
                                &pUrlPtr->UrlPort) == TRUE)
        {
                if((pUrlPtr->UrlHost.nLength - pUrlPtr->UrlPort.nLength) < 10)
                {
                        // To-Do: check the actual port length before the memcpy
                        pUrlPtr->UrlPort.pParam += pUrlPtr->UrlPort.nLength + 1;
                        memcpy(UrlPort,pUrlPtr->UrlPort.pParam,15);
                        pUrlPtr->UrlPort.nLength = 0;
                        pPtr = UrlPort;
                        while(*pPtr && pPtr++)
                        {

                                pUrlPtr->UrlPort.nLength++;
                                if(*pPtr == '/')
                                {
                                        *pPtr = 0;
                                        pUrlPtr->nPort = (UINT16)atol(UrlPort);
                                        pHTTPSession->HttpFlags = pHTTPSession->HttpFlags | HTTP_CLIENT_FLAG_URLANDPORT;
                                        break;
                                }
                        }
                }
                else
                {
                        // Port too big
                        return HTTP_CLIENT_ERROR_BAD_URL;
                }
        }

        // Get the request body
        pUrlPtr->UrlRequest.pParam  = pUrlPtr->Url + nUrlOffset;
        pUrlPtr->UrlRequest.nLength = nUrlLength - nUrlOffset;
        HC_DBG(("url Request:%s",pUrlPtr->UrlRequest.pParam));

        // If we got SSL url with no port we should set the default ssl port
        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_URLHTTPS) == HTTP_CLIENT_FLAG_URLHTTPS)
        {
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_URLANDPORT) != HTTP_CLIENT_FLAG_URLANDPORT)
                {
                        pHTTPSession->HttpUrl.nPort = HTTP_CLIENT_DEFAULT_SSL_PORT;

                }

        }
        HC_DBG(("Request port:%d",pHTTPSession->HttpUrl.nPort));
        // Set the state flag
        pHTTPSession->HttpState  = pHTTPSession->HttpState | HTTP_CLIENT_STATE_URL_PARSED;

        return HTTP_CLIENT_SUCCESS;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnResizeBuffer
// Purpose      : Resize the HTTP headers buffer and reset the pointers
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnResizeBuffer (P_HTTP_SESSION pHTTPSession,
                UINT32 nNewBufferSize)    // [IN] The new (and larger) buffer size
{

        CHAR                *pPtr           = NULL;
        UINT32              nCurrentBufferSize;

        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
        // If the new buffer size is less or equal to the current buffer size then..
        if(pHTTPSession->HttpHeaders.HeadersBuffer.nLength >= nNewBufferSize)
        {

                // Return an error (bad buffer)
                return HTTP_CLIENT_ERROR_BUFFER_RSIZE;
        }

        // If the new buffer size is bigger then the defined maximum buffer size then..
        if(nNewBufferSize > HTTP_CLIENT_MAX_SEND_RECV_HEADERS)
        {
                // Return an error (bad buffer)
                return HTTP_CLIENT_ERROR_BUFFER_RSIZE;
        }
        // Current buffer size is the sum of the incoming and outgoing headers strings lengths
        nCurrentBufferSize = pHTTPSession->HttpHeaders.HeadersOut.nLength + pHTTPSession->HttpHeaders.HeadersIn.nLength;
        // Allocate a new buffer with the requested buffer size
        pPtr = (CHAR*)malloc(ALIGN(nNewBufferSize));
        if(!pPtr)
        {
                // malloc() error
                return HTTP_CLIENT_ERROR_NO_MEMORY;
        }

        // Copy the memory only if there is data to copy
        if(nCurrentBufferSize > 0)
        {
                memcpy(pPtr,pHTTPSession->HttpHeaders.HeadersBuffer.pParam,nCurrentBufferSize);
                // Reset the rest of the buffer
                memset(pPtr + nCurrentBufferSize, 0x00,(nNewBufferSize - nCurrentBufferSize));
        }
        else
        {
                // Reset the entire buffer (no previous buffer was copied)
                memset(pPtr,0x00,nNewBufferSize);
        }

        free(pHTTPSession->HttpHeaders.HeadersBuffer.pParam);

        pHTTPSession->HttpHeaders.HeadersBuffer.pParam = pPtr;
        pHTTPSession->HttpHeaders.HeadersBuffer.nLength = nNewBufferSize;

        // Refresh the pointers
        pHTTPSession->HttpHeaders.HeadersOut.pParam = pHTTPSession->HttpHeaders.HeadersBuffer.pParam;
        if(pHTTPSession->HttpHeaders.HeadersIn.pParam)
        {
                pHTTPSession->HttpHeaders.HeadersIn.pParam  =pHTTPSession->HttpHeaders.HeadersBuffer.pParam + pHTTPSession->HttpHeaders.HeadersOut.nLength;
        }
        return HTTP_CLIENT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnRemoveHeader
// Purpose      : Removes an HTTP headers by its name
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersRemove (P_HTTP_SESSION pHTTPSession,
                CHAR *pHeaderName)   // [IN] The header's name

{

        HTTP_PARAM  HttpParam;
        UINT32      nRetCode = HTTP_CLIENT_SUCCESS;
        UINT32      nBytes;

        if(!pHTTPSession) // Pointer validation check
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        // First see if we have that header in our outgoing headers buffer
        do
        {

                if((nRetCode = HTTPIntrnHeadersFind(pHTTPSession,pHeaderName,&HttpParam,FALSE,0)) != HTTP_CLIENT_SUCCESS)
                {
                        // Could not find this header
                        break;
                }
                // Calculate the new headers length
                nBytes = (HttpParam.pParam - pHTTPSession->HttpHeaders.HeadersOut.pParam);
                nBytes -= HttpParam.nLength;


                // Copy the memory
                memcpy(HttpParam.pParam, HttpParam.pParam + HttpParam.nLength,nBytes);

                // Set the new length
                pHTTPSession->HttpHeaders.HeadersOut.nLength -= HttpParam.nLength;

                // Reset the buffer from it's modified end to it's previous end
                memset(pHTTPSession->HttpHeaders.HeadersOut.pParam + pHTTPSession->HttpHeaders.HeadersOut.nLength,0x00,HttpParam.nLength);

        } while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnHeadersRemove",NULL,0,"Removing Header %",pHeaderName);
        }
#endif


        return nRetCode;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnHeadersAdd
// Purpose      : Add HTTP headers to the outgoing headers buffers
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersAdd (P_HTTP_SESSION pHTTPSession,
                CHAR *pHeaderName,   // [IN] The header's name
                UINT32 nNameLength,  // [IN] Name length
                CHAR *pHeaderData,   // [IN] The Header's data
                UINT32 nDataLength)  // [IN] Data length
{
        CHAR            *pPtr;
        UINT32          nProjectedHeaderLength;
        UINT32          nProjectedBufferLength;
        INT32           nCurrentfreeSpace;
        INT32           nProjectedfreeSpace;
        UINT32          nRetCode;

        if(!pHTTPSession) // pointer validation check
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
        HC_DBG(("Add headers(%.*s):%.*s",(int)nNameLength,pHeaderName,(int)nDataLength,pHeaderData));
        nProjectedHeaderLength  = nNameLength + nDataLength + 4;
        nProjectedBufferLength  = nProjectedHeaderLength + pHTTPSession->HttpHeaders.HeadersOut.nLength + pHTTPSession->HttpHeaders.HeadersIn.nLength;
        nCurrentfreeSpace       = pHTTPSession->HttpHeaders.HeadersBuffer.nLength - (pHTTPSession->HttpHeaders.HeadersOut.nLength + pHTTPSession->HttpHeaders.HeadersIn.nLength);
        nProjectedfreeSpace     = nCurrentfreeSpace - nProjectedHeaderLength;

        // Check total size limit
        if(nProjectedBufferLength > HTTP_CLIENT_MAX_SEND_RECV_HEADERS)
        {
                return HTTP_CLIENT_ERROR_NO_MEMORY;
        }

        if((INT32)nProjectedfreeSpace < 0)
        {
                if(HTTP_CLIENT_MEMORY_RESIZABLE == FALSE)
                {
                        // Need more space but we can't grow beyond the current size
                        return HTTP_CLIENT_ERROR_NO_MEMORY;
                }
                else
                {
                        // We can resizes so..
                        nRetCode = HTTPIntrnResizeBuffer(pHTTPSession,nProjectedBufferLength + HTTP_CLIENT_MEMORY_RESIZE_FACTOR);
						if(nRetCode != HTTP_CLIENT_SUCCESS)
                        {
                                return nRetCode;
                        }
                }
        }
        // Move the incoming headers data within the buffer so we will have space for the added headers
        if(pHTTPSession->HttpHeaders.HeadersIn.pParam)
        {
                // Move the data and reset the data in the offset.
                memmove(pHTTPSession->HttpHeaders.HeadersIn.pParam + nProjectedHeaderLength  ,
                                pHTTPSession->HttpHeaders.HeadersIn.pParam,
                                pHTTPSession->HttpHeaders.HeadersIn.nLength);
                // Reset the space created
                memset(pHTTPSession->HttpHeaders.HeadersOut.pParam + pHTTPSession->HttpHeaders.HeadersOut.nLength,
                                0x00,
                                nProjectedHeaderLength);

        }

        pPtr = pHTTPSession->HttpHeaders.HeadersOut.pParam + pHTTPSession->HttpHeaders.HeadersOut.nLength;
        // Create the new header
        memcpy(pPtr,pHeaderName,nNameLength);
        pPtr += nNameLength;
        memcpy(pPtr,": ",2);
        pPtr += 2;
        memcpy(pPtr,pHeaderData,nDataLength);
        pPtr += nDataLength;
        memcpy(pPtr,HTTP_CLIENT_CRLF,2);
        pPtr += 2;

        // Set the new length
        pHTTPSession->HttpHeaders.HeadersOut.nLength += nProjectedHeaderLength;

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnHeadersAdd",NULL,0,"Adding Header %s: %s",pHeaderName,pHeaderData);
        }
#endif

        return HTTP_CLIENT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnConnectionClose
// Purpose      : Closes an active socket connection and invalidate the socket handle
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnConnectionClose (P_HTTP_SESSION pHTTPSession)
{

        INT32  nRetCode = HTTP_CLIENT_SUCCESS;
        do
        {
                if(!pHTTPSession)   // Validate the session pointer
                {
                        nRetCode =  HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // If we have a valid socket then..
                if(pHTTPSession->HttpConnection.HttpSocket != HTTP_INVALID_SOCKET)// INVALID_SOCKET
                {
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SECURE) == HTTP_CLIENT_FLAG_SECURE)
                        {
                                // TLS Close
                                nRetCode = HTTPWrapperSSLClose(pHTTPSession->HttpConnection.HttpSocket);
                        }
                        // Gracefully close it
#ifdef _WIN32
                        shutdown(pHTTPSession->HttpConnection.HttpSocket,0x01);
                        closesocket(pHTTPSession->HttpConnection.HttpSocket);
#elif _LINUX
                        shutdown(pHTTPSession->HttpConnection.HttpSocket,0x01);
                        close(pHTTPSession->HttpConnection.HttpSocket);
#else
                        closesocket(pHTTPSession->HttpConnection.HttpSocket);
#endif
                        // And invalidate the socket
                        pHTTPSession->HttpConnection.HttpSocket = HTTP_INVALID_SOCKET;

                        break;;
                }
                else
                {
                        // Not a valid socket error
                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_INVALID;
                        break;
                }

        } while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnConnectionClose",NULL,0,"");
        }
#endif
        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnConnectionOpen
// Purpose      : Opens a socket connection to the remote host or proxy server
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnConnectionOpen (P_HTTP_SESSION pHTTPSession)
{
        INT32            nRetCode = HTTP_CLIENT_SUCCESS;     // a function return code value
        UINT32           nNullOffset;                        // a helper value to null terminate a given string
        int              nNonBlocking    = 1;                // non blocking mode parameter
        CHAR             Backup;                             // a container for a char value (helps in temporary null termination)
        // HTTP_HOSTNET     *HostEntry;                          // Socket host entry pointer
        UINT32           Address = 0;
        HTTP_SOCKADDR_IN ServerAddress;                      // Socket address structure
        HTTP_SOCKADDR_IN LoaclAddress;                       // Socket address structure (for client binding)
        HC_DBG(("Connection..\n"));
        do
        {

                if(!pHTTPSession)
                {
                        nRetCode  =  HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }
                // Use an existing connection if valid
                if(pHTTPSession->HttpConnection.HttpSocket != HTTP_INVALID_SOCKET)
                {
                        // Set the state flag
                        pHTTPSession->HttpState  = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HOST_CONNECTED;
                        return HTTP_CLIENT_SUCCESS;
                }
                // Zero the socket events
                FD_ZERO(&pHTTPSession->HttpConnection.FDRead);
                FD_ZERO(&pHTTPSession->HttpConnection.FDWrite);
                FD_ZERO(&pHTTPSession->HttpConnection.FDError);

                if(pHTTPSession->HttpConnection.HttpSocket == HTTP_INVALID_SOCKET)
                {

                        // Create a TCP/IP stream socket
                        pHTTPSession->HttpConnection.HttpSocket = socket(AF_INET,	    // Address family
                                        SOCK_STREAM,			                    // Socket type
                                        IPPROTO_TCP);		                        // Protocol
                }

                // Exit if we don't have a valid socket
                if(pHTTPSession->HttpConnection.HttpSocket == HTTP_INVALID_SOCKET)
                {
                        HC_ERR(("Create socket failed.."));
                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_INVALID;
                        break;
                }
                HC_DBG(("Create local socket: %lu",pHTTPSession->HttpConnection.HttpSocket));
                // Set non blocking socket
#ifdef _WIN32
                nRetCode = ioctlsocket(pHTTPSession->HttpConnection.HttpSocket, FIONBIO, &nNonBlocking);
#else
                nRetCode = sktSetNonblocking(pHTTPSession->HttpConnection.HttpSocket , nNonBlocking);
#endif
                if(nRetCode != 0)
                {
                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_CANT_SET;
                        break;
                }
#ifdef HTTPC_PROXY
                // Resolve the target host name
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_USINGPROXY) != HTTP_CLIENT_FLAG_USINGPROXY)
                {
#endif
                        // No proxy, directly resolving the host name
                        // Prep the parameter
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_URLANDPORT) == HTTP_CLIENT_FLAG_URLANDPORT)
                        {
                                nNullOffset = pHTTPSession->HttpUrl.UrlHost.nLength - pHTTPSession->HttpUrl.UrlPort.nLength - 1;
                        }
                        else
                        {
                                nNullOffset = pHTTPSession->HttpUrl.UrlHost.nLength;
                        }

                        Backup = HTTPStrExtract(pHTTPSession->HttpUrl.UrlHost.pParam,nNullOffset,0);
                        // Resolve the host name
                        nRetCode = HostByName(pHTTPSession->HttpUrl.UrlHost.pParam,(unsigned long*)&Address);

                        // Restore from backup (fix the buffer)
                        HTTPStrExtract(pHTTPSession->HttpUrl.UrlHost.pParam,nNullOffset,Backup);

#ifdef HTTPC_PROXY
                }
                else
                {
                        // Using a Proxy server so resolve the proxy host name
                        nRetCode = HostByName(pHTTPSession->HttpProxy.ProxyHost,(unsigned long*)&Address);
                }
#endif
                // See if we have a valid response from the net resolve operation
                /*
                   if(nRetCode)
                   {
                   nRetCode = HTTP_CLIENT_ERROR_SOCKET_RESOLVE;
                   break;
                   }
                   */
                // Reset the address structures
                memset(&ServerAddress, 0, sizeof(HTTP_SOCKADDR_IN));
                memset(&LoaclAddress, 0, sizeof(HTTP_SOCKADDR_IN));

                // Fill in the address structure
                ServerAddress.sin_family        = AF_INET;
                ServerAddress.sin_addr.s_addr   = Address;       // Server's address
#ifdef HTTPC_PROXY
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_USINGPROXY) != HTTP_CLIENT_FLAG_USINGPROXY)

                {
#endif

                        // Use the remote web server port
                        ServerAddress.sin_port      = htons(pHTTPSession->HttpUrl.nPort);        // Host Port number
#ifdef HTTPC_PROXY
                }
                else
                {
                        // Use the proxy port
                        ServerAddress.sin_port      = htons(pHTTPSession->HttpProxy.nProxyPort);  // Proxy Port number
                }
#endif

                // Client-side Binding
                if(pHTTPSession->HttpConnection.HttpClientPort != 0)
                {
                        LoaclAddress.sin_family         = AF_INET;
                        LoaclAddress.sin_port           = htons((unsigned short)pHTTPSession->HttpConnection.HttpClientPort);

                        nRetCode = bind(pHTTPSession->HttpConnection.HttpSocket,
                                        (HTTP_SOCKADDR*)&LoaclAddress,
                                        sizeof(HTTP_SOCKADDR_IN));

                        if(nRetCode != 0)
                        {
                                nRetCode = HTTP_CLIENT_ERROR_SOCKET_BIND;
                        }
                }

                // Connect using TLS or otherwise clear connection
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SECURE) == HTTP_CLIENT_FLAG_SECURE)
                { // Is it a TLS connection?
                        CHAR* HostName = (CHAR *)malloc(pHTTPSession->HttpUrl.UrlHost.nLength+1);
                        memset(HostName, 0, pHTTPSession->HttpUrl.UrlHost.nLength+1);
                        memcpy(HostName, pHTTPSession->HttpUrl.UrlHost.pParam, pHTTPSession->HttpUrl.UrlHost.nLength);
                        HC_DBG(("connect using TLS...|%s|..|%s| (%d)\n",
                                HostName,
                                inet_ntoa(ServerAddress.sin_addr),
                                (int)(pHTTPSession->HttpConnection.HttpSocket)));
                        nRetCode = HTTPWrapperSSLConnect(pHTTPSession->HttpConnection.HttpSocket,	// Socket
                                        (HTTP_SOCKADDR*)&ServerAddress,         // Server address
                                        sizeof(HTTP_SOCKADDR),                  // Length of server address structure
                                        HostName);	                        // Hostname
                        free(HostName);
                }
                else    // Non TLS so..
                {
                        nRetCode = connect(pHTTPSession->HttpConnection.HttpSocket,	// Socket
                                        (HTTP_SOCKADDR*)&ServerAddress,			                // Server address
                                        sizeof(HTTP_SOCKADDR));		                    // Length of server address structure
                }

                // The socket was set to be asyn so we should check the error being returned from connect()
#ifdef _WIN32
                nRetCode = SocketGetErr(pHTTPSession->HttpConnection.HttpSocket);
#else
                if (nRetCode == -1)
                        nRetCode = HTTPC_ERRNO;
#endif
                if(nRetCode == 0 || nRetCode == HTTP_EWOULDBLOCK || nRetCode == HTTP_EINPROGRESS)
                {

                        // Set TLS Nego flag to flase
                        pHTTPSession->HttpConnection.TlsNego = FALSE;
                        // Set the Write fd_sets for a socket connection event
                        FD_SET(pHTTPSession->HttpConnection.HttpSocket, &pHTTPSession->HttpConnection.FDWrite);
                        // Set the state flag
                        pHTTPSession->HttpState  = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HOST_CONNECTED;
                        // We have connected so set the return value to success
                        nRetCode = HTTP_CLIENT_SUCCESS;
                        break;
                }
                else
                {
                        // Socket connection problem
                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_CONNECT;
                        break;
                }
        }while(0);
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnConnectionOpen",NULL,0,"");
        }
#endif

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnSend
// Purpose      : Send data to the remote server (Asynchronous sockets)
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnSend (P_HTTP_SESSION pHTTPSession,
                CHAR *pData,            // [IN] a pointer to the data to be sent
                UINT32 *nLength)        // [IN OUT] Length of data to send and the transmitted bytes count
{

        INT32           nSocketEvents;                   // Socket events center
        INT32           nRetCode            = HTTP_CLIENT_SUCCESS;  // a function return code value
        HTTP_TIMEVAL    Timeval             = { 1 , 0 };  // Timeout value for the socket() method
        HTTP_CONNECTION *pConnection        = NULL;      // Pointer for the connection structure

        INT32           nSendSize = 0;

        do
        {
                // Validate the session pointer
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Have a pointer on the internal connection structure for simplifying code reading
                pConnection = &pHTTPSession->HttpConnection;

                while(1)
                {

                        // Check for timeout
                        if(HTTPIntrnSessionEvalTimeout(pHTTPSession) == TRUE)
                        {
                                nRetCode = HTTP_CLIENT_ERROR_SOCKET_TIME_OUT;
                                break;
                        }

                        // Reset socket events , only Error, since we don't want to get
                        // a repeated Write events (socket is connected)

                        FD_SET(pConnection->HttpSocket, &pConnection->FDError);

#ifdef HTTPC_LWIP
                        FD_SET(pConnection->HttpSocket, &pConnection->FDWrite);

#endif
                        // See if we got any events on the socket

                        nSocketEvents = select((pConnection->HttpSocket + 1), 0,
                                        &pConnection->FDWrite,
                                        &pConnection->FDError,
                                        &Timeval);
                        if(nSocketEvents < 0) // No events on the socket
                        {
                                *(nLength) = 0;
                                break; // To-Do: This might be an error
                        }

                        if(nSocketEvents == 0) // No new events so
                        {
                                continue; // restart this loop
                        }


                        // Socket is writable (we are connected) so send the data
                        if(FD_ISSET(pConnection->HttpSocket ,&pConnection->FDWrite))
                        {

                                FD_CLR((UINT32)pConnection->HttpSocket,&pConnection->FDWrite);
                                // Send the data
                                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SECURE) == HTTP_CLIENT_FLAG_SECURE)
                                {
                                        // TLS Protected connection
                                        if(pConnection->TlsNego == FALSE)
                                        {
                                                nRetCode = HTTPWrapperSSLNegotiate(pConnection->HttpSocket,0,0,"desktop");
                                                if(nRetCode != 0)
                                                {
                                                        // TLS Error
                                                        nRetCode = HTTP_CLIENT_ERROR_TLS_NEGO;
                                                        break;
                                                }
                                                pConnection->TlsNego = TRUE;
                                        }
                                        nRetCode = HTTPWrapperSSLSend(pConnection->HttpSocket,pData,*(nLength),0);
                                }
                                else
                                {
                                        nRetCode = send(pConnection->HttpSocket,pData+nSendSize,*(nLength)-nSendSize,0);
                                }

                                if(nRetCode == SOCKET_ERROR)
                                {
                                        nRetCode = SocketGetErr(pHTTPSession->HttpConnection.HttpSocket);
                                        nRetCode = HTTP_CLIENT_ERROR_SOCKET_SEND;
                                        break;
                                }
                                else
                                {
                                    nSendSize += nRetCode;
                                    if(nSendSize != *(nLength))
                                        continue;
                                }
                                // The data was sent to the remote server
                                *(nLength) = nSendSize;
                                nRetCode = HTTP_CLIENT_SUCCESS;
                                break;
                        }

                        // We had a socket related error
                        if(FD_ISSET(pConnection->HttpSocket ,&pConnection->FDError))
                        {
                                FD_CLR((UINT32)pConnection->HttpSocket,&pConnection->FDError);
                                *(nLength) = 0;
                                // To-Do: Handle this case
                                nRetCode = HTTP_CLIENT_ERROR_SOCKET_SEND;
                                break;
                        }

                }
        } while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnSend",pData,*(nLength),"");
        }
#endif


        return nRetCode;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnRecv
// Purpose      : Receive data from the connected socket using asynchronous sockets
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnRecv (P_HTTP_SESSION pHTTPSession,
                CHAR *pData,        // [IN] a pointer for a buffer that receives the data
                UINT32 *nLength,    // [IN OUT] Length of the buffer and the count of the received bytes
                BOOL PeekOnly)      // [IN] State if we should only peek the socket (default is no)
{
        INT32           nSocketEvents;
        INT32           nRetCode = HTTP_CLIENT_SUCCESS;
        HTTP_TIMEVAL    Timeval         = { 0, 50000 };
        HTTP_CONNECTION *pConnection     = NULL;

        do
        {
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Set a pointer on the session internal connection structure (simplify code reading)
                pConnection = &pHTTPSession->HttpConnection;
                while(1)
                {
                        // Check for timeout
                        if(HTTPIntrnSessionEvalTimeout(pHTTPSession) == TRUE)
                        {

                                nRetCode =  HTTP_CLIENT_ERROR_SOCKET_TIME_OUT;
                                break;
                        }

						///////////////////////////////////////////////////////////////////////////////////////////////////////////
						// This is a simple bypass for the TSL session (for some reason the socket read event is not set so
						// The pending bytes on the socket are being checked manualy.
						// TLS hack:
						if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SECURE) == HTTP_CLIENT_FLAG_SECURE)
						{
								// Recive without being notified by the socket event
								if((nRetCode = HTTPWrapperSSLRecv(pConnection->HttpSocket,pData,*(nLength),0)) == SOCKET_ERROR)
								{
										// Socket error
										nRetCode =	HTTP_CLIENT_ERROR_SOCKET_RECV;
										break;
								}
								*(nLength) = nRetCode;
								// Break on no data or server connection reset
#ifdef _WIN32
								if ( nRetCode == 0 || nRetCode == HTTP_ECONNRESET)
#else
										if ( nRetCode == 0 /*|| nRetCode == HTTP_ECONNRESET*/)
#endif
										{
												// Connection closed, simply break - this is not an error
												nRetCode =	HTTP_CLIENT_EOS;  // Signal end of stream
												break;
										}
								// We have successfully got the data from the server
								nRetCode = HTTP_CLIENT_SUCCESS;
								break;
						}


                        // Reset socket events
                        FD_SET(pConnection->HttpSocket, &pConnection->FDRead);
                        FD_SET(pConnection->HttpSocket, &pConnection->FDError);

                        // See if we got any events on the socket
                        nSocketEvents = select(pConnection->HttpSocket + 1, &pConnection->FDRead,
                                        0,
                                        &pConnection->FDError,
                                        &Timeval);

                        if(nSocketEvents < 0) // Error or no new socket events
                        {
                                *(nLength) = 0;
                                break;
                        }

                        if(FD_ISSET(pConnection->HttpSocket ,&pConnection->FDRead)) // Are there any read events on the socket ?
                        {

                                // Clear the event
                                FD_CLR((UINT32)pConnection->HttpSocket,&pConnection->FDRead);

                                // Socket is readable so so read the data
                                if(PeekOnly == FALSE)
                                {

                                        if((nRetCode = recv(pConnection->HttpSocket,pData,*(nLength),0)) == SOCKET_ERROR)
                                        {
                                                // Socket error

                                                nRetCode =  HTTP_CLIENT_ERROR_SOCKET_RECV;
                                                break;
                                        }

                                }
                                else
                                {
                                        // Only peek te socket
                                        if((nRetCode = recv(pConnection->HttpSocket,pData,*(nLength),MSG_PEEK)) == SOCKET_ERROR)
                                        {
                                                // Socket error
                                                nRetCode =  HTTP_CLIENT_ERROR_SOCKET_RECV;
                                                break;
                                        }

                                }
                                *(nLength) = nRetCode;
                                // Break on no data or server connection reset
                                // MSDN: If the connection has been gracefully closed, the return value is zero.
#ifdef _WIN32
                                if ( nRetCode == 0 || nRetCode == HTTP_ECONNRESET)
#else
                                        if ( nRetCode == 0 /*|| nRetCode == HTTP_ECONNRESET*/)
#endif
                                        {
                                                // Connection closed, simply break - this is not an error
                                                nRetCode =  HTTP_CLIENT_EOS;  // Signal end of stream
                                                break;
                                        }
                                // We have successfully got the data from the server
                                nRetCode = HTTP_CLIENT_SUCCESS;
                                break;
                        }
                        // We had a socket related error
                        if(FD_ISSET(pConnection->HttpSocket ,&pConnection->FDError))
                        {
                                FD_CLR((UINT32)pConnection->HttpSocket,&pConnection->FDError);
                                *(nLength) = 0;

                                // To-Do: Handle this case
                                nRetCode = HTTP_CLIENT_ERROR_SOCKET_RECV;
                                break;

                        }
                }
        }while(0);

        return nRetCode;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnGetRemoteChunkLength
// Purpose      : Receive (byte by byte) the chunk parameter (while in chunk mode receive) and
//                Convert the HEX string into an integer
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnGetRemoteChunkLength (P_HTTP_SESSION pHTTPSession)
{

        UINT32          nBytesRead = 1;
        UINT32          nRetCode = HTTP_CLIENT_SUCCESS;
        UINT32          nBytesCount = 0;
        CHAR            ChunkHeader[HTTP_CLIENT_MAX_CHUNK_HEADER];
        CHAR            *pPtr;

        do
        {
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Read byte by byte until we get a CrLf
                pPtr = ChunkHeader; // Get a pointer to the received buffer
                *pPtr = 0;          // Terminate with null

                while(nBytesRead > 0)
                {
                        // Receive a single byte
                        nRetCode = HTTPIntrnRecv(pHTTPSession,pPtr,&nBytesRead,FALSE);
                        // Did we succeed?
                        if(nRetCode == HTTP_CLIENT_SUCCESS && nBytesRead > 0)
                        {
                                // Increment the bytes count
                                nBytesCount += nBytesRead;
                                if(nBytesRead > HTTP_CLIENT_MAX_CHUNK_HEADER)
                                {
                                        // Error chunk buffer is full
                                        nRetCode = HTTP_CLIENT_ERROR_CHUNK_TOO_BIG;
                                        break;
                                }
                                // Don't Process if the fist 2 bytes are CrLf.
                                if(! ((nBytesCount == 1 && *pPtr == 0x0d) || (nBytesCount == 2 && *pPtr == 0x0a)))
                                {
                                        // Advance the pointer by the received data length
                                        pPtr += nBytesRead;
                                        // Look for CrLf in the last 2 bytes
                                        if(memcmp(pPtr - 2,HTTP_CLIENT_CRLF,2) == 0)
                                        {
                                                // Chunk Header was received
                                                *pPtr = 0;  // null terminate the chunk parameter
                                                pHTTPSession->HttpCounters.nRecivedChunkLength = HTTPStrHToL(ChunkHeader); // Convert to a number
                                                // Set the HTTP counters
                                                pHTTPSession->HttpCounters.nBytesToNextChunk =  pHTTPSession->HttpCounters.nRecivedChunkLength;
                                                break;
                                        }
                                }
                        }
                        else // Socket Error
                        {
                                nRetCode = 0;
                                break;
                        }
                }
        } while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnGetRemoteChunkLength",NULL,0,"Next chunk is %d bytes",pHTTPSession->HttpCounters.nRecivedChunkLength);
        }
#endif

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnGetRemoteHeaders
// Purpose      : Perform a socket receive (byte by byte) until all the HTTP headers are received
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnGetRemoteHeaders (P_HTTP_SESSION pHTTPSession)
{

        UINT32          nBytesRead = 1;
        UINT32          nRetCode = HTTP_CLIENT_SUCCESS;
        UINT32          nProjectedHeaderLength;
        UINT32          nProjectedBufferLength;
        INT32           nCurrentfreeSpace;
        INT32           nProjectedfreeSpace;
        CHAR            *pPtr;

        do
        {
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Read byte by byte until we get CrLf followed by CrLf
                // Set the incoming headers pointer

                if(!pHTTPSession->HttpHeaders.HeadersIn.pParam)
                {
                        /// The incoming headers starts where the outgoing headers ends
                        pHTTPSession->HttpHeaders.HeadersIn.pParam = pHTTPSession->HttpHeaders.HeadersOut.pParam + pHTTPSession->HttpHeaders.HeadersOut.nLength;
                }

                // Receive until we get all the headers or any other error event
                while(nBytesRead > 0)
                {
                        // Size of the projected buffer we are going to receive
                        nProjectedHeaderLength  = nBytesRead;
                        // Size of the projected total incoming buffer
                        nProjectedBufferLength  = nProjectedHeaderLength + pHTTPSession->HttpHeaders.HeadersOut.nLength + pHTTPSession->HttpHeaders.HeadersIn.nLength;
                        // Current free space on the incoming headers buffer
                        nCurrentfreeSpace       = pHTTPSession->HttpHeaders.HeadersBuffer.nLength - (pHTTPSession->HttpHeaders.HeadersOut.nLength + pHTTPSession->HttpHeaders.HeadersIn.nLength);
                        // Projected free space after the completion of the receive
                        nProjectedfreeSpace     = nCurrentfreeSpace - nProjectedHeaderLength;

                        // Check total size limit
                        if(nProjectedBufferLength > HTTP_CLIENT_MAX_SEND_RECV_HEADERS)
                        {
                                return HTTP_CLIENT_ERROR_NO_MEMORY;
                        }

                        if((INT32)nProjectedfreeSpace < 0)
                        {
                                if(HTTP_CLIENT_MEMORY_RESIZABLE == FALSE)
                                {
                                        // Need more space but we can't grow beyond the current size
                                        nRetCode = HTTP_CLIENT_ERROR_NO_MEMORY;
                                        break;
                                }
                                else
                                {
                                        // We can resizes so..
                                        nRetCode = HTTPIntrnResizeBuffer(pHTTPSession,nProjectedBufferLength + HTTP_CLIENT_MEMORY_RESIZE_FACTOR);
										if(nRetCode != HTTP_CLIENT_SUCCESS)
                                        {
                                                break;
                                        }
                                }
                        }
                        // Jump to the beginning of the incoming headers (just after the end of the outgoing headers)
                        pPtr = pHTTPSession->HttpHeaders.HeadersIn.pParam + pHTTPSession->HttpHeaders.HeadersIn.nLength;
                        // Read a single byte
                        nRetCode = HTTPIntrnRecv(pHTTPSession,pPtr,&nBytesRead,FALSE);
                        // ToDo: Break if not getting HTTP on the first 4 bytes

                        if(nRetCode == HTTP_CLIENT_SUCCESS && nBytesRead > 0)
                        {
                                // Advance the pointer by 1 byte
                                pPtr += nBytesRead;
                                // Increase the total receive length
                                pHTTPSession->HttpHeaders.HeadersIn.nLength++;

                                // Set the HTTP counters
                                pHTTPSession->HttpCounters.nRecivedHeaderLength++;

                                if(memcmp(pPtr - 4,HTTP_CLIENT_CRLFX2,4) == 0)
                                {
                                        // Headers were received
                                        HC_DBG(("Response Header:\n%.*s",(int)pHTTPSession->HttpCounters.nRecivedHeaderLength,pHTTPSession->HttpHeaders.HeadersIn.pParam));
					HTTPWrapperGetRespHeader(pHTTPSession->HttpHeaders.HeadersIn.pParam);
					HC_DBG(("http resp content length:%ld\n", HTTPWrapperGetRespContentLength()));
                                        break;
                                }
                        }
                        else
                        {
                                nRetCode =  HTTP_CLIENT_ERROR_HEADER_RECV; // This was marked out for some reason
                                break;
                        }
                }
        }while(0);


#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnGetRemoteHeaders",NULL,0,"Got %d bytes",pHTTPSession->HttpHeaders.HeadersIn.nLength);
        }
#endif
        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnHeadersFind
// Purpose      : Look for a header (insensitive search) by its name
// Gets         : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersFind (P_HTTP_SESSION pHTTPSession,CHAR *pHeaderName,
                HTTP_PARAM *pParam,     //  [OUT] HTTP parameter structure that holds the search results
                BOOL IncommingHeaders,  //  [IN]  Indicate if we are to search in the incoming or outgoing headers
                UINT32 nOffset)         //  [IN]  Optionaly privide an offset to start looking from
{
        CHAR           *pHeaderEnd;
#ifdef HTTPC_LITTLE_STACK
        CHAR           *Header =NULL;
        if (!(Header = malloc(HTTP_CLIENT_MAX_HEADER_SEARCH_CLUE)))
                return HTTP_CLIENT_ERROR_NO_MEMORY;
#else
        CHAR           Header[HTTP_CLIENT_MAX_HEADER_SEARCH_CLUE]; // To-Do: Use pointers insted of fixed length
#endif


        UINT32         nLength;
        UINT32         nRetCode = HTTP_CLIENT_ERROR_HEADER_NOT_FOUND;

        do
        {
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                // Reset the input parameter structure
                pParam->pParam  = NULL;
                pParam->nLength = 0;
                // Get the requested header length
                nLength = strlen(pHeaderName);
                if(nLength > (HTTP_CLIENT_MAX_HEADER_SEARCH_CLUE - 3))
                {
                        // Error : header search clue too big
                        nRetCode = HTTP_CLIENT_ERROR_HEADER_BIG_CLUE;
                        break;
                }
                // Build the searched header name , add a leading CrLf before the header name and trailing ":"
                memset(Header,0x00,HTTP_CLIENT_MAX_HEADER_SEARCH_CLUE);
                strcpy(Header,HTTP_CLIENT_CRLF);
                strcat(Header,pHeaderName);
                strcat(Header,":");
                // Case insensitive search for the header name (search the incoming headers)
                if(IncommingHeaders == TRUE)
                {
                        pParam->pParam = HTTPStrCaseStr(pHTTPSession->HttpHeaders.HeadersIn.pParam + nOffset,
                                        pHTTPSession->HttpHeaders.HeadersIn.nLength,
                                        Header);
                }
                else
                {
                        // Optionally search the outgoing headers
                        pParam->pParam = HTTPStrCaseStr(pHTTPSession->HttpHeaders.HeadersOut.pParam + nOffset,
                                        pHTTPSession->HttpHeaders.HeadersOut.nLength,
                                        Header);
                }

                if(pParam->pParam) // Did we find it?
                {
                        // Search for the token end (trailing CrLf)
                        pHeaderEnd = strstr(pParam->pParam + 2,HTTP_CLIENT_CRLF);
                        if(pHeaderEnd)
                        {
                                // Get the length (up to the CrLf)
                                pParam->nLength =  pHeaderEnd -  pParam->pParam;
                                nRetCode = HTTP_CLIENT_SUCCESS;
                                break;

                        }
                }
        }while(0);
#ifdef HTTPC_LITTLE_STACK
        if (Header != NULL)
                free(Header);
#endif
        // Could not find the header
        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnAuthenticate
// Purpose      :
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnAuthenticate(P_HTTP_SESSION pHTTPSession)
{
        UINT32      nRetCode = HTTP_CLIENT_SUCCESS;   // Function call return code
        UINT32      nBytes = 32;
        UINT32      nTotalBytes = 0;
        CHAR        ErrorPage[32];
        BOOL        NewConnection = FALSE;


        do
        {
                // Validate the session pointer
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

                //  Handle connection close message (reconnect)
                if(pHTTPSession->HttpHeadersInfo.Connection == FALSE)
                {
                        // Gracefully close the connection and set the socket as invalid
                        if(pHTTPSession->HttpConnection.HttpSocket != HTTP_INVALID_SOCKET)
                        {
                                HTTPIntrnConnectionClose(pHTTPSession);
                        }
                        // Connect to the remote server (or proxy)
                        nRetCode = HTTPIntrnConnectionOpen(pHTTPSession);
                        if(nRetCode != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }

                        NewConnection = TRUE;
                }

                // Analyze the security headers and optionally build the authentication reply header
                if((nRetCode = HTTPIntrnParseAuthHeader(pHTTPSession))!= HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // We have to recive any HTML data here inorder to "Clear" the socket buffer for later usage
                // Note: We should skip this when the HEAD verb was used
                while(NewConnection == FALSE && pHTTPSession->HttpHeaders.HttpLastVerb != VerbHead && pHTTPSession->HttpHeadersInfo.nHTTPContentLength > 0 && nBytes > 0)
                {
                        ErrorPage[0] = 0;
                        if((nRetCode = HTTPIntrnRecv(pHTTPSession,ErrorPage,&nBytes,FALSE)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }

                        nTotalBytes += nBytes;
                        if(nTotalBytes >= pHTTPSession->HttpHeadersInfo.nHTTPContentLength)
                        {
                                break;
                        }
                }

                // Re-Send the headers after having analyzed the authorizaton headers
                if((nRetCode = HTTPIntrnHeadersSend(pHTTPSession,pHTTPSession->HttpHeaders.HttpVerb)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

        }while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnAuthenticate",NULL,0,"");
        }
#endif

        return nRetCode;

}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnHeadersParse
// Purpose      : Parse the HTTP incoming headers.
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersParse (P_HTTP_SESSION pHTTPSession)
{

        CHAR            *pPtr;                                      // a pointer that points on the incoming headers
        UINT32          nTokenLength = 0;                           // Length of the parsed token
        UINT32          nRetCode = HTTP_CLIENT_SUCCESS;             // a function return code value
        UINT32          nOffset = 0;                                // Bytes offset (strings comperision)


#ifdef HTTPC_LITTLE_STACK
        CHAR           *HTTPToken =NULL;
        if (!(HTTPToken = malloc(HTTP_CLIENT_MAX_TOKEN_LENGTH)))
                return HTTP_CLIENT_ERROR_NO_MEMORY;
#else
        CHAR            HTTPToken[HTTP_CLIENT_MAX_TOKEN_LENGTH];    // Buffer for the parsed HTTP token
#endif


        HTTP_PARAM      HTTPParam;                                  // A generic pointer\length parameter for parsing
        BOOL            AuthHeaders = FALSE;                        // While we are searching the authentication methods

        do
        {
                // Validate the session pointer
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        pHTTPSession->pDebug("HTTPIntrnHeadersParse",pHTTPSession->HttpHeaders.HeadersIn.pParam,pHTTPSession->HttpHeaders.HeadersIn.nLength,"[Incomming Headers]");
                }
#endif


                // Set a pointer on the incoming headers
                pPtr = pHTTPSession->HttpHeaders.HeadersIn.pParam;

                // Detect the leading HTTP string
                if(HTTPStrInsensitiveCompare(pPtr,"http",4) != TRUE)
                {
                        nRetCode = HTTP_CLIENT_ERROR_BAD_HEADER;
                        break;
                }

                // Get the HTTP Version
                while ((*pPtr) && (*pPtr != 0x20))
                {
                        nTokenLength++;
                        pPtr++; // Move to the first space
                }
                strncpy(pHTTPSession->HttpHeadersInfo.HTTPVersion,
                                pPtr - nTokenLength,
                                MIN(15,nTokenLength));
                pPtr++;

                // Get the HTTP status code
                memset(HTTPToken,0x00,HTTP_CLIENT_MAX_TOKEN_LENGTH);
                nTokenLength = 0;
                while ((*pPtr) && (*pPtr != 0x20))
                {
                        nTokenLength++;
                        pPtr++; // Move to the next space
                }
                strncpy(HTTPToken,(pPtr - nTokenLength),MIN(HTTP_CLIENT_MAX_TOKEN_LENGTH,nTokenLength));

                pHTTPSession->HttpHeadersInfo.nHTTPStatus = atol(HTTPToken);

                // Search for content length
                pHTTPSession->HttpHeadersInfo.nHTTPContentLength = 0; // Default no unknown length
                // Look for the token
                if(HTTPIntrnHeadersFind(pHTTPSession,"content-length",&HTTPParam,TRUE,0) == HTTP_CLIENT_SUCCESS)
                {

                        memset(HTTPToken,0x00,HTTP_CLIENT_MAX_TOKEN_LENGTH);        // Reset the token buffer
                        nTokenLength  = HTTP_CLIENT_MAX_TOKEN_LENGTH;               // Set the buffer length
                        // Attempt to extract the token
                        if(HTTPStrGetToken(HTTPParam.pParam,HTTPParam.nLength,HTTPToken,&nTokenLength))
                        {
                                // Convert the content-length into an integer.
                                pHTTPSession->HttpHeadersInfo.nHTTPContentLength = atol(HTTPToken);
                        }
                }

                // Search for connection status
                pHTTPSession->HttpHeadersInfo.Connection = TRUE; // Default status where no server connection header was detected
                // Look for token (can be standard connection or a proxy connection)
                if( (HTTPIntrnHeadersFind(pHTTPSession,"connection",&HTTPParam,TRUE,0) == HTTP_CLIENT_SUCCESS) ||
                                (HTTPIntrnHeadersFind(pHTTPSession,"proxy-connection",&HTTPParam,TRUE,0) == HTTP_CLIENT_SUCCESS))
                {

                        memset(HTTPToken,0x00,HTTP_CLIENT_MAX_TOKEN_LENGTH);
                        nTokenLength  = HTTP_CLIENT_MAX_TOKEN_LENGTH;
                        // Attempt to extract the token
                        if(HTTPStrGetToken(HTTPParam.pParam,HTTPParam.nLength,HTTPToken,&nTokenLength))
                        {
                                // Is this a keep alive session?
                                pHTTPSession->HttpHeadersInfo.Connection = HTTPStrInsensitiveCompare(HTTPToken,"keep-alive",0);
                                // Is it a closed session
                                if(HTTPStrInsensitiveCompare(HTTPToken,"close",0) == TRUE)
                                {
                                        pHTTPSession->HttpHeadersInfo.Connection = FALSE;
                                }
                        }
                }

                // Search for chunking mode transfer
                pHTTPSession->HttpFlags = pHTTPSession->HttpFlags &~ HTTP_CLIENT_FLAG_CHUNKED; // Remove the flag
                if(HTTPIntrnHeadersFind(pHTTPSession,"transfer-encoding",&HTTPParam,TRUE,0) == HTTP_CLIENT_SUCCESS)
                {

                        memset(HTTPToken,0x00,HTTP_CLIENT_MAX_TOKEN_LENGTH);
                        nTokenLength  = HTTP_CLIENT_MAX_TOKEN_LENGTH;
                        if(HTTPStrGetToken(HTTPParam.pParam,HTTPParam.nLength,HTTPToken,&nTokenLength))
                        {
                                // If the chunks token was find then set the session flag accordingly
                                if(HTTPStrInsensitiveCompare(HTTPToken,"chunked",0) == TRUE)
                                {
                                        pHTTPSession->HttpFlags  =  pHTTPSession->HttpFlags  | HTTP_CLIENT_FLAG_CHUNKED;
                                }
                        }
                }
                // Look for the authentication header
                while(AuthHeaders == FALSE)  // address multiple authentication methods presented by the server
                {
                        if(pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_UNAUTHORIZED)
                        {
                                // Double check for the "www-authenticate" header token
                                if(HTTPIntrnHeadersFind(pHTTPSession,"www-authenticate",&pHTTPSession->HttpAuthHeader.AuthHeader,TRUE,nOffset) != HTTP_CLIENT_SUCCESS)
                                {
                                        if(nOffset > 0) // an authentication header was found but not the right one so adjust the error
                                        {
                                                nRetCode = HTTP_CLIENT_ERROR_AUTH_MISMATCH;
                                        }
                                        else
                                        {
                                                nRetCode = HTTP_CLIENT_ERROR_BAD_HEADER;
                                        }

                                        break;
                                }

                                // Make sure that we get an authentication header that maches the caller requested schema
                                pPtr = HTTPStrCaseStr(pHTTPSession->HttpAuthHeader.AuthHeader.pParam,
                                                pHTTPSession->HttpAuthHeader.AuthHeader.nLength,
                                                pHTTPSession->HttpCredentials.AuthSchemaName);
                                if(pPtr)
                                {
                                        AuthHeaders = TRUE;
                                }
                                else
                                {
                                        // Simply pass the point where the last "www" was found
                                        nOffset = (pHTTPSession->HttpAuthHeader.AuthHeader.pParam - pHTTPSession->HttpHeaders.HeadersIn.pParam) + 3;
                                }
                        }
                        else
                        {
                                AuthHeaders = TRUE;
                        }
                }
#ifdef HTTPC_PROXY
                // Is this a proxy authentication header?
                if(pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED)
                {
                        // Double check for the "Proxy-Authentication" header token
                        if (HTTPIntrnHeadersFind(pHTTPSession,"proxy-authenticate",&pHTTPSession->HttpAuthHeader.AuthHeader,TRUE,0) != HTTP_CLIENT_SUCCESS)
                        {
                                nRetCode = HTTP_CLIENT_ERROR_BAD_HEADER;
                                break;
                        }
                }
#endif
                // Do we have a redirection response?
                if( (pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_OBJECT_MOVED) ||
                                (pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_OBJECT_MOVED_PERMANENTLY))
                {
                        // Check for the "Location" header token
                        if (HTTPIntrnHeadersFind(pHTTPSession,"location",&pHTTPSession->HttpHeadersInfo.HttpRedirectURL,TRUE,0) != HTTP_CLIENT_SUCCESS)
                        {
                                // Protocol violation, we got a redirect code without the host name to redirect to
                                nRetCode = HTTP_CLIENT_ERROR_BAD_HEADER;
                                break;
                        }
                        // Fix the pointers location (address the "Location: " prefix)
                        pHTTPSession->HttpHeadersInfo.HttpRedirectURL.pParam += 12;
                        pHTTPSession->HttpHeadersInfo.HttpRedirectURL.nLength -= 12;

                }

        }while(0);

#ifdef HTTPC_LITTLE_STACK
        if (HTTPToken != NULL)
                free(HTTPToken);
#endif
        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnParseAuthHeader
// Purpose      : Parse the HTTP headers for the required authentication method
// Gets         :
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnParseAuthHeader(P_HTTP_SESSION pHTTPSession)
{

        CHAR            *pPtrStart, *pPtrEnd;

        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef HTTPC_PROXY
        if(pHTTPSession->HttpProxy.ProxyAuthSchema != AuthSchemaNone)
        {
                // for proxy authentication simply assume basic and exit
                return HTTP_CLIENT_SUCCESS;
        }
#endif
        // Advance the pointer in the string and break on the first space
        pPtrEnd   = pHTTPSession->HttpAuthHeader.AuthHeader.pParam + pHTTPSession->HttpAuthHeader.AuthHeader.nLength;
        pPtrStart = pHTTPSession->HttpAuthHeader.AuthHeader.pParam;
        // Jump to the first space
        while ((pPtrEnd - pPtrStart) > 0 && *pPtrStart != 0x20) pPtrStart++;

        do
        {
                if(HTTPStrCaseStr(pPtrStart,8,"basic"))
                {
                        pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA = AuthSchemaBasic;
                        break;
                }

                if(HTTPStrCaseStr(pPtrStart,8,"digest"))
                {
                        pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA = AuthSchemaDigest;
                        break;
                }
                if(HTTPStrCaseStr(pPtrStart,8,"negotiate")) // Note that this could be NLM negotiation as well (which is not supported)
                {
                        pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA = AuthSchemaKerberos;
                        break;
                }
                // To-Do: Add any other supported authentication method
        }
        while(0);

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnParseAuthHeader",pHTTPSession->HttpAuthHeader.AuthHeader.pParam,
                                pHTTPSession->HttpAuthHeader.AuthHeader.nLength,"[Incomming Auth Headers: %d]",pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA);
        }
#endif

        // If we could not detect the authentication schema return an error
        if(pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA == AuthSchemaNone)
        {
                return HTTP_CLIENT_ERROR_BAD_AUTH;
        }

        //Make sure we are going to authenticate with the method specified by the caller
        if(pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA != (UINT32)pHTTPSession->HttpCredentials.CredAuthSchema)
        {
                return HTTP_CLIENT_ERROR_AUTH_MISMATCH;
        }


        return HTTP_CLIENT_SUCCESS;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnHeadersSend
// Purpose      : Build and send the HTTP request. this includes the HTTP headers
//                and any required authentication data
// Gets         :
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersSend(P_HTTP_SESSION pHTTPSession,
                HTTP_VERB HttpVerb)  // [IN] Argument that can bypass the requested verb
        // Can be used for evaluating a HEAD request
{

        UINT32          nBytes;
        UINT32          nRetCode = HTTP_CLIENT_SUCCESS;
#ifdef HTTPC_SEND_TOGTHER
        CHAR            *RequestCmd = NULL;
        UINT32         nAllocationSize;             // Size of the dynamically allocated buffer
#else
        CHAR            RequestCmd[16];
#endif
        CHAR            ContentLength[32];
        BOOL            RestoreHeadersFlag = FALSE;
        HTTP_VERB       HttpCachedVerb;

        if(!pHTTPSession)
        {
                // Bad session pointer error
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef HTTPC_SEND_TOGTHER
        if(HTTP_CLIENT_MEMORY_RESIZABLE)
        {
                // Memory is resizable, so use the init defined size or the maximum buffer size (which ever is smaller)
                //nAllocationSize = MIN(HTTP_CLIENT_MAX_SEND_RECV_HEADERS,HTTP_CLIENT_INIT_SEND_RECV_HEADERS);
				nAllocationSize = pHTTPSession->HttpHeaders.HeadersBuffer.nLength;
        }
        else
        {
                // Memory is not resizable so simply use the maximum defined size
                nAllocationSize = HTTP_CLIENT_MAX_SEND_RECV_HEADERS;
        }

        RequestCmd = (CHAR *)malloc(nAllocationSize);

        // Did we succeed?
        if(!RequestCmd)
        {
                // Null pointer is returned upon error
                return HTTP_CLIENT_ERROR_NO_MEMORY;
        }
#endif
#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnHeadersSend",NULL,
                                0,"Using Verb: %d",(INT32)HttpVerb);
        }
#endif
        // Cache the original VERB
        HttpCachedVerb = pHTTPSession->HttpHeaders.HttpVerb;

        do
        {

                // Set the verb (temporarily)
                if(pHTTPSession->HttpHeaders.HttpVerb != HttpVerb)
                {
                        if((nRetCode = HTTPClientSetVerb((HTTP_SESSION_HANDLE)(unsigned long)pHTTPSession,HttpVerb)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }

                // Remeber this state for later usage
                pHTTPSession->HttpHeaders.HttpLastVerb =  pHTTPSession->HttpHeaders.HttpVerb;

                // If this is a head request we should temporary remove the chunking header and the content length header
                if(pHTTPSession->HttpHeaders.HttpVerb == VerbHead)
                {

                        // If send in chunks flag was set
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SEND_CHUNKED) == HTTP_CLIENT_FLAG_SEND_CHUNKED)
                        {
                                // Chunking
                                if((nRetCode = HTTPIntrnHeadersRemove(pHTTPSession,"Transfer-Encoding")) != HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }
                        // Content-Length
                        if(pHTTPSession->HttpHeadersInfo.nHTTPPostContentLength > 0) // Attempt to remove only if it was previusly set
                        {
                                if((nRetCode = HTTPIntrnHeadersRemove(pHTTPSession,"Content-Length")) != HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }

                        RestoreHeadersFlag = TRUE; // So it would be restored later
                }
                // Request Verb
                nBytes = strlen(pHTTPSession->HttpHeaders.Verb) + 1;
#ifdef HTTPC_SEND_TOGTHER
                memset(RequestCmd,0x00,nAllocationSize);
#else
                memset(RequestCmd,0x00,16);
#endif
                strcpy(RequestCmd,pHTTPSession->HttpHeaders.Verb);
                strcat(RequestCmd," ");
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,RequestCmd,&nBytes)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#endif
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;

#ifdef HTTPC_PROXY
                // Request URI
                if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_USINGPROXY) != HTTP_CLIENT_FLAG_USINGPROXY)

                {
#endif
                        nBytes = pHTTPSession->HttpUrl.UrlRequest.nLength;
#ifndef HTTPC_SEND_TOGTHER
                        if((nRetCode = HTTPIntrnSend(pHTTPSession,pHTTPSession->HttpUrl.UrlRequest.pParam,&nBytes)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
#else
                        if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                                HC_ERR(("Request Header is too large,%d",__LINE__));
                                break;
                        }
                        strncpy(RequestCmd + pHTTPSession->HttpCounters.nSentHeaderBytes,pHTTPSession->HttpUrl.UrlRequest.pParam,nBytes);
#endif
                        // Set the counters
                        pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;
#ifdef HTTPC_PROXY
                }
                else
                {
                        nBytes = strlen(pHTTPSession->HttpUrl.Url);
#ifndef HTTPC_SEND_TOGTHER
                        if((nRetCode = HTTPIntrnSend(pHTTPSession,pHTTPSession->HttpUrl.Url,&nBytes)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
#else
                        if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                                HC_ERR(("Request Header is too large,%d",__LINE__));
                                break;
                        }
                        strncpy(RequestCmd + pHTTPSession->HttpCounters.nSentHeaderBytes,pHTTPSession->HttpUrl.Url,nBytes);
#endif
                        // Set the counters
                        pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;
                }
#endif
                // Request HTTP Version
#ifndef HTTPC_SEND_TOGTHER
                memset(RequestCmd,0x00,16);
#else
                nBytes = strlen(HTTP_CLIENT_DEFAULT_VER)+strlen(HTTP_CLIENT_CRLF)+strlen(" ");
                if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                        HC_ERR(("Request Header is too large,%d",__LINE__));
                        break;
                }
#endif
                strcat(RequestCmd," ");
                strcat(RequestCmd,HTTP_CLIENT_DEFAULT_VER);
                strcat(RequestCmd,HTTP_CLIENT_CRLF);

#ifndef HTTPC_SEND_TOGTHER
                nBytes = strlen(RequestCmd);
                if((nRetCode = HTTPIntrnSend(pHTTPSession,RequestCmd,&nBytes)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#endif
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;

                // Request headers
                nBytes = pHTTPSession->HttpHeaders.HeadersOut.nLength;
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,pHTTPSession->HttpHeaders.HeadersOut.pParam,&nBytes)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#else

                if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                        HC_ERR(("Request Header is too large,%d",__LINE__));
                        break;
                }
                strncpy(RequestCmd + pHTTPSession->HttpCounters.nSentHeaderBytes,pHTTPSession->HttpHeaders.HeadersOut.pParam,nBytes);
#endif

                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;

                // Optionally add authentication headers and send them (for host or proxy authentication)
                if(pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_UNAUTHORIZED ||
#ifdef HTTPC_PROXY
                                pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED  )
#else
                        0)
#endif
                        {
#ifdef HTTPC_SEND_TOGTHER
                                pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam = malloc(HTTP_CLIENT_MAX_SEND_RECV_HEADERS);
                                if (!(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam)) {
                                        HC_ERR(("AUTH malloc failed,%s",__func__));
                                        break;
                                }
                                pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength = HTTP_CLIENT_MAX_SEND_RECV_HEADERS;
                                memset(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam,0 ,HTTP_CLIENT_MAX_SEND_RECV_HEADERS);

#endif
                                if((nRetCode = HTTPIntrnAuthHandler(pHTTPSession)) != HTTP_CLIENT_SUCCESS)
                                {
#ifdef HTTPC_SEND_TOGTHER
                                        HC_ERR(("AUTH perform failed : %lu",nRetCode));
                                        free(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam);
                                        pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam = NULL;
#endif
                                        break;
                                }
#ifdef HTTPC_SEND_TOGTHER
                                nBytes = pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength;
                                if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                                        free(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam);
                                        pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam = NULL;
                                        HC_ERR(("Request Header is too large.(%d,%d,%d)",(int)(pHTTPSession->HttpCounters.nSentHeaderBytes), (int)nBytes, (int)nAllocationSize));
                                        break;
                                }
                                HC_DBG(("AuthInfo Param:%s\n\n",pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam));
                                strncpy(RequestCmd + pHTTPSession->HttpCounters.nSentHeaderBytes,pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam,nBytes);
                                pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;
                                if (pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam != NULL) {
                                        free(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam);
                                        pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam = NULL;
                                        pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength = 0;
                                }
#endif
                        }

                // Request terminating CrLf
                nBytes = strlen(HTTP_CLIENT_CRLF);

#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HTTP_CLIENT_CRLF,&nBytes)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#else
                if (pHTTPSession->HttpCounters.nSentHeaderBytes + nBytes >= nAllocationSize) {
                        HC_ERR(("Request Header is too large,%d",__LINE__));
                        break;
                }
                strncpy(RequestCmd + pHTTPSession->HttpCounters.nSentHeaderBytes,HTTP_CLIENT_CRLF,nBytes);

#endif
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nBytes;

                HC_DBG(("Request Header:(line:%d)\n%s",__LINE__,RequestCmd));
#ifdef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,RequestCmd,&pHTTPSession->HttpCounters.nSentHeaderBytes)) != HTTP_CLIENT_SUCCESS)
                {
                        pHTTPSession->HttpCounters.nSentHeaderBytes = 0;
                        break;
                }
#endif
                // Restore the verb
                if(pHTTPSession->HttpHeaders.HttpVerb != HttpCachedVerb)
                {
                        if((nRetCode = HTTPClientSetVerb((HTTP_SESSION_HANDLE)(unsigned long)pHTTPSession,HttpCachedVerb)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                }

                if(RestoreHeadersFlag == TRUE)
                {
                        // Restore chunking header (since it was temporarily removed for the head request
                        // Add the  Transfer-Encoding:  header
                        if((pHTTPSession->HttpFlags & HTTP_CLIENT_FLAG_SEND_CHUNKED) == HTTP_CLIENT_FLAG_SEND_CHUNKED)
                        {
                                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Transfer-Encoding",17,"chunked",7))!= HTTP_CLIENT_SUCCESS)
                                {
                                        break;
                                }
                        }
                        // Restore the content length
                        if(pHTTPSession->HttpHeadersInfo.nHTTPPostContentLength > 0) // Attempt to remove only if it was previusly set
                        {
                                IToA(ContentLength,pHTTPSession->HttpHeadersInfo.nHTTPPostContentLength); // Convert the buffer length to a string value
                                if((nRetCode = HTTPIntrnHeadersAdd(pHTTPSession,"Content-Length",14,ContentLength,strlen(ContentLength)))!= HTTP_CLIENT_SUCCESS)
                                {
                                        return nRetCode;
                                }
                        }
                }
                // Set the session stage
                pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_REQUEST_SENT;

        } while(0);
#ifdef HTTPC_SEND_TOGTHER
        if (RequestCmd != NULL)
                free(RequestCmd);

#endif
        return nRetCode;     // end of HTTPIntrnSendHeaders()
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnAuthHandler
// Purpose      : Differentiate between the authenticate method that we have to implement and perform
//                the required operation.
// Gets         :
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnAuthHandler (P_HTTP_SESSION pHTTPSession)
{

        UINT32 nRetCode = HTTP_CLIENT_SUCCESS;

        if(!pHTTPSession)
        {
                // Bad session pointer error
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }
#ifdef HTTPC_PROXY
        if(pHTTPSession->HttpProxy.ProxyAuthSchema != AuthSchemaNone)
        {
                // For proxy authentication simply assume basic and exit
                // Basic authentication
                nRetCode = HTTPIntrnAuthSendBasic(pHTTPSession);
                return nRetCode;
        }
#endif

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnAuthHandler",NULL,
                                0,"");
        }
#endif

        // Use the correct authentication method as requested by the server
        switch(pHTTPSession->HttpAuthHeader.HTTP_AUTH_SCHEMA)
        {

                case AuthSchemaBasic:
                        {
                                // Basic authentication
                                nRetCode = HTTPIntrnAuthSendBasic(pHTTPSession);
                                break;

                        }
                case AuthSchemaDigest:
                        {
                                // Digest authentication
                                nRetCode = HTTPIntrnAuthSendDigest(pHTTPSession);
                                break;

                        }
                case AuthSchemaKerberos:
                        {
                                // ToDo: impliment the Kerberos nego authentication here
                                nRetCode = HTTP_CLIENT_ERROR_NOT_IMPLEMENTED;
                                break;

                        }
                default:
                        {
                                // Not supported method
                                return HTTP_CLIENT_ERROR_BAD_AUTH; // Not implemented error
                        }

        };

        // This session requested an authentication so..
        pHTTPSession->HttpCredentials.Authentication = TRUE;
        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnAuthSendBasic
// Purpose      : Handle basic authentication for direst host connection and proxy authentication
// Gets         :
// Returns      :
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////


UINT32 HTTPIntrnAuthSendBasic (P_HTTP_SESSION pHTTPSession)
{

        UINT32      nSegmentLength;
#ifdef HTTPC_SEND_TOGTHER
        UINT32      nRetCode = HTTP_CLIENT_SUCCESS;
        UINT32      nbytes ,tbytes;
#else
        UINT32      nRetCode;
#endif
        CHAR        Cred[HTTP_CLIENT_MAX_64_ENCODED_CRED /2];   // Credentials (Clear)
        CHAR        Cred64[HTTP_CLIENT_MAX_64_ENCODED_CRED];    // Credentials (64 bit encoded)
        UINT32      nSrcLength, nDestLength;
        CHAR*       pPtr;
        CHAR*       INITIAL_HDR         = "Authorization: Basic ";
#ifdef HTTPC_PROXY
        CHAR*       INITIAL_PROXY_HDR   = "Proxy-Authorization: Basic ";
#endif

        do
        {
                if(!pHTTPSession)
                {
                        nRetCode = HTTP_CLIENT_ERROR_INVALID_HANDLE;
                        break;
                }

#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        pHTTPSession->pDebug("HTTPIntrnAuthSendBasic",NULL,
                                        0,"");
                }
#endif

                memset(Cred,0x00,HTTP_CLIENT_MAX_64_ENCODED_CRED /2);
                memset(Cred64,0x00,HTTP_CLIENT_MAX_64_ENCODED_CRED);

                switch (pHTTPSession->HttpHeadersInfo.nHTTPStatus)
                {
                        case( HTTP_STATUS_UNAUTHORIZED): // For host authentication
                                {

                                        // Copy the clear text credentials to a format of user:password
                                        strcpy(Cred,pHTTPSession->HttpCredentials.CredUser);
                                        strcat(Cred,":");
                                        strcat(Cred,pHTTPSession->HttpCredentials.CredPassword);
                                        nSrcLength  = strlen(Cred);



                                        nDestLength = HTTP_CLIENT_MAX_64_ENCODED_CRED;
                                        nSegmentLength = strlen(INITIAL_HDR);
#ifndef HTTPC_SEND_TOGTHER
                                        // Build and send the data first the hard-coded static portion
                                        if((nRetCode = HTTPIntrnSend(pHTTPSession,INITIAL_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                                        {
                                                break;
                                        }
                                        // Set the counters
                                        pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else

                                        tbytes = nbytes = nSegmentLength;
                                        if (tbytes >= pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength) {
                                                HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes,pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength));
                                                nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                                                break;
                                        }

                                        strncpy(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam,INITIAL_HDR,nbytes);
#endif
                                        // Convert to base 64
                                        HTTPBase64Encoder((unsigned char *)Cred64,(const unsigned char *)Cred,nSrcLength);
                                        nDestLength = strlen(Cred64);

                                };
                                break;
#ifdef HTTPC_PROXY
                        case (HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED): // For Proxy authentication
                                {
                                        // Copy the clear text credentials to a format of user:password
                                        strcpy(Cred,pHTTPSession->HttpProxy.ProxtUser);
                                        strcat(Cred,":");
                                        strcat(Cred,pHTTPSession->HttpProxy.ProxyPassword);
                                        nSrcLength  = strlen(Cred);
                                        nDestLength = HTTP_CLIENT_MAX_64_ENCODED_CRED;

                                        // Convert to base 64
                                        HTTPBase64Encoder((unsigned char *)Cred64,(unsigned char *)Cred,nSrcLength);
                                        nDestLength = strlen(Cred64);
                                        nSegmentLength = strlen(INITIAL_PROXY_HDR);
                                        // Build and send the data first the hard-coded static portion
                                        if((nRetCode = HTTPIntrnSend(pHTTPSession,INITIAL_PROXY_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                                        {
                                                break;
                                        }
                                        // Set the counters
                                        pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;

                                };
                                break;
#endif
                        default:
                                {
                                        return HTTP_CLIENT_ERROR_BAD_AUTH; // Wrong status for this function
                                };
                };

                // Send the base 64 encoded data
                pPtr = Cred64;
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,pPtr, &nDestLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nDestLength;
#else

                nbytes = nDestLength;
                if (tbytes + nbytes >= pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes,pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam + tbytes,pPtr,nbytes);
                tbytes += nbytes;

#endif
                // Terminating CRLF
                nSegmentLength = strlen(HTTP_CLIENT_CRLF);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HTTP_CLIENT_CRLF, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else

                nbytes = nSegmentLength;
                if (tbytes + nbytes >= pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes,pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam + tbytes,HTTP_CLIENT_CRLF,nbytes);
                tbytes += nbytes;
                pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength = tbytes;
#endif


        } while (0);

        return nRetCode;

}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnAuthSendDigest
// Purpose      : Handle digest authentication for direct host connection and proxy authentication
// Gets         :
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnAuthSendDigest (P_HTTP_SESSION pHTTPSession)
{
        CHAR        Cnonce[33];
        UINT32      nSegmentLength;
        UINT32      nRetCode;
        UINT32      nAlgType = 0; // a flag for the algorithem type (default to MD5)
        HTTP_PARAM  HttpParamOpq,HttpParamRealm,HttpParamNonce,HttpParamQop,HttpParamAlg;     // Pointers and lengths of the dynamic sections
        // of the Digest response.
#ifdef HTTPC_SEND_TOGTHER
        UINT32      nbytes ,tbytes;
        CHAR        *pParam = pHTTPSession->HttpCredentials.ToSendAuthInfo.pParam;
        if (!pParam) {
                HC_ERR(("pParam is NULL"));
                return HTTP_CLIENT_UNKNOWN_ERROR;
        }
        UINT32        nLength = pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength;
#endif

        // Fragments of the Digest client response (The hard coded text portion of the response)
        CHAR*       INITIAL_HDR         = "Authorization: Digest username=\"";
#ifdef HTTPC_PROXY
        CHAR*       INITIAL_PROXY_HDR   = "Proxy-Authorization: Digest username=\"";
#endif
        CHAR*       REALEM_HDR          = "\", realm=\"";
        CHAR*       QOP_HDR             = "\", qop=\"";
        CHAR*       ALGO_HDR            = "\", algorithm=\"";
        CHAR*       URI_HDR             = "\", uri=\"";
        CHAR*       NONCE_HDR           = "\", nonce=\"";
        CHAR*       NC_HDR              = "\", nc=00000001, cnonce=\""; // To-Do: This should be tested!!
        CHAR*       RSP_HDR             = "\", response=\"";
        CHAR*       OPQ_HDR             = "\", opaque=\"";
        // Digest Calculation related
        HASHHEX     HA1;
        HASHHEX     HA2 = "";
        HASHHEX     Response;

        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnAuthSendDigest",NULL,
                                0,"");
        }
#endif

        // Generate random Cnonce number
        HTTPDigestGenerateCNonce(Cnonce);

        switch (pHTTPSession->HttpHeadersInfo.nHTTPStatus)
        {
                case( HTTP_STATUS_UNAUTHORIZED): // For host authentication
                        {
                                // "Authorization: Digest username="
                                nSegmentLength = strlen(INITIAL_HDR);
#ifndef HTTPC_SEND_TOGTHER
                                if((nRetCode = HTTPIntrnSend(pHTTPSession,INITIAL_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                                {
                                        return nRetCode;
                                }
                                // Set the counters
                                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                                tbytes = nbytes = nSegmentLength;
                                if (tbytes >= nLength) {
                                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes,nLength));
                                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                                        break;
                                }
                                strncpy(pParam,INITIAL_HDR,nbytes);
                                pParam += nbytes;
#endif

                        };
                        break;
#ifdef HTTPC_PROXY
                case (HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED): // For Proxy authentication
                        {
                                // "Proxy-Authorization: Digest username="
                                nSegmentLength = strlen(INITIAL_PROXY_HDR);
                                if((nRetCode = HTTPIntrnSend(pHTTPSession,INITIAL_PROXY_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                                {
                                        return nRetCode;
                                }
                                // Set the counters
                                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
                        };
                        break;
#endif
                default:
                        {
                                return HTTP_CLIENT_ERROR_BAD_AUTH; // Wrong status for this function
                        };
        };

        do
        {

                // "Authorization: Digest username="username
                nSegmentLength = strlen(pHTTPSession->HttpCredentials.CredUser);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,pHTTPSession->HttpCredentials.CredUser, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,pHTTPSession->HttpCredentials.CredUser,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif

                // "Authorization: Digest username="username", realm="
                nSegmentLength = strlen(REALEM_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,REALEM_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;

#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,REALEM_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif

                // "Authorization: Digest username="username", realm="realm
                if((nRetCode = HTTPStrGetDigestToken(pHTTPSession->HttpAuthHeader.AuthHeader,"realm", &HttpParamRealm)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HttpParamRealm.pParam, &HttpParamRealm.nLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += HttpParamRealm.nLength;
#else
                nbytes = HttpParamRealm.nLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,HttpParamRealm.pParam,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                nSegmentLength = strlen(QOP_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,QOP_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,QOP_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                if((nRetCode = HTTPStrGetDigestToken(pHTTPSession->HttpAuthHeader.AuthHeader,"qop", &HttpParamQop)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HttpParamQop.pParam, &HttpParamQop.nLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += HttpParamQop.nLength;
#else
                nbytes = HttpParamQop.nLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,HttpParamQop.pParam,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5",

                nSegmentLength = strlen(ALGO_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,ALGO_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,ALGO_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                if((nRetCode = HTTPStrGetDigestToken(pHTTPSession->HttpAuthHeader.AuthHeader,"algorithm", &HttpParamAlg)) != HTTP_CLIENT_SUCCESS)
                {

                        // The server did not state its required algorithm so use the default
                        HttpParamAlg.pParam  =   HTTP_CLIENT_DEFAULT_DIGEST_AUTH;
                        HttpParamAlg.nLength =   strlen(HTTP_CLIENT_DEFAULT_DIGEST_AUTH);
                }
                nRetCode = 0;
                // Get the algorithem type
                if(HTTPStrInsensitiveCompare(HttpParamAlg.pParam ,"md5",3 ) == TRUE)
                {
                        if(HTTPStrInsensitiveCompare(HttpParamAlg.pParam ,"md5-sess", HttpParamAlg.nLength) == TRUE)
                        {
                                nAlgType = 1;
                        }

                }
                else
                {
                        // Error algorithem not supported
                        nRetCode = HTTP_CLIENT_ERROR_NO_DIGEST_ALG;
                        break;
                }
#ifndef HTTPC_SEND_TOGTHER

                // Send the algorithem
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HttpParamAlg.pParam, &HttpParamAlg.nLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += HttpParamAlg.nLength;
#else
                nbytes = HttpParamAlg.nLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,HttpParamAlg.pParam,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif

                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="
                nSegmentLength = strlen(URI_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,URI_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,URI_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service
                nSegmentLength = strlen(pHTTPSession->HttpUrl.UrlRequest.pParam);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,pHTTPSession->HttpUrl.UrlRequest.pParam, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,pHTTPSession->HttpUrl.UrlRequest.pParam,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="
                nSegmentLength = strlen(NONCE_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,NONCE_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,NONCE_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="7a5c...
                if((nRetCode = HTTPStrGetDigestToken(pHTTPSession->HttpAuthHeader.AuthHeader,"nonce", &HttpParamNonce)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HttpParamNonce.pParam, &HttpParamNonce.nLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += HttpParamNonce.nLength;
#else
                nbytes = HttpParamNonce.nLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,HttpParamNonce.pParam,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="7a5c...", nc=00000001, cnonce="
                nSegmentLength = strlen(NC_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,NC_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }

                strncpy(pParam,NC_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="7a5c...", nc=00000001, cnonce="ab341...
                nSegmentLength = strlen(Cnonce);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,Cnonce, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >=  nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,Cnonce,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // Send the opaque data if we got it from the server
                if((nRetCode = HTTPStrGetDigestToken(pHTTPSession->HttpAuthHeader.AuthHeader,"opaque", &HttpParamOpq)) == HTTP_CLIENT_SUCCESS)
                {

                        nSegmentLength = strlen(OPQ_HDR);
#ifndef HTTPC_SEND_TOGTHER
                        if((nRetCode = HTTPIntrnSend(pHTTPSession,OPQ_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                        // Set the counters
                        pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                        nbytes = nSegmentLength;
                        if (tbytes + nbytes >= nLength) {
                                HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                                nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                                break;
                        }
                        strncpy(pParam,OPQ_HDR,nbytes);
                        tbytes += nbytes;
                        pParam += nbytes;
#endif

#ifndef HTTPC_SEND_TOGTHER
                        if((nRetCode = HTTPIntrnSend(pHTTPSession,HttpParamOpq.pParam, &HttpParamOpq.nLength)) != HTTP_CLIENT_SUCCESS)
                        {
                                break;
                        }
                        // Set the counters
                        pHTTPSession->HttpCounters.nSentHeaderBytes += HttpParamOpq.nLength;
#else
                        nbytes = HttpParamOpq.nLength;
                        if (tbytes + nbytes >= nLength) {
                                HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                                nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                                break;
                        }
                        strncpy(pParam,HttpParamOpq.pParam,nbytes);
                        tbytes += nbytes;
                        pParam += nbytes;
#endif
                }
                nRetCode = 0;
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="7a5c...", nc=00000001, cnonce="ab341...", response="
                nSegmentLength = strlen(RSP_HDR);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,RSP_HDR, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,RSP_HDR,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // Calculate response
                HTTPDigestCalcHA1(nAlgType, pHTTPSession->HttpCredentials.CredUser,
                                HttpParamRealm.pParam,HttpParamRealm.nLength ,
                                pHTTPSession->HttpCredentials.CredPassword ,
                                HttpParamNonce.pParam, HttpParamNonce.nLength,
                                Cnonce, HA1);
                HTTPDigestCalcResponse(HA1,
                                HttpParamNonce.pParam, HttpParamNonce.nLength,
                                "00000001", Cnonce,
                                HttpParamQop.pParam,HttpParamQop.nLength, pHTTPSession->HttpHeaders.Verb,
                                pHTTPSession->HttpUrl.UrlRequest.pParam,pHTTPSession->HttpUrl.UrlRequest.nLength,
                                HA2, Response);
                // "Authorization: Digest username="username", realm="myRealm", qop="auth",
                // algorithm="MD5", uri="/....Service", nonce="7a5c...", nc=00000001, cnonce="ab341...", response="8bbf2...
                nSegmentLength = strlen(Response);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,Response, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,Response,nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif

                // Terminate 0x24 (")
                nSegmentLength = 1;
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,"\"", &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,"\"",nbytes);
                tbytes += nbytes;
                pParam += nbytes;
#endif
                // Terminating CRLF
                nSegmentLength = strlen(HTTP_CLIENT_CRLF);
#ifndef HTTPC_SEND_TOGTHER
                if((nRetCode = HTTPIntrnSend(pHTTPSession,HTTP_CLIENT_CRLF, &nSegmentLength)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }
                // Set the counters
                pHTTPSession->HttpCounters.nSentHeaderBytes += nSegmentLength;
#else
                nbytes = nSegmentLength;
                if (tbytes + nbytes >= nLength) {
                        HC_ERR(("auth info is too long(%d).(%lu,%lu)",__LINE__,tbytes, nLength));
                        nRetCode = HTTP_CLIENT_UNKNOWN_ERROR;
                        break;
                }
                strncpy(pParam,HTTP_CLIENT_CRLF,nbytes);
                tbytes += nbytes;

                pHTTPSession->HttpCredentials.ToSendAuthInfo.nLength = tbytes;
#endif

        } while(0);

        return nRetCode; // End of digest respobse sending
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnSessionReset
// Purpose      : Reset the session data for the next operation
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnSessionReset (P_HTTP_SESSION pHTTPSession, BOOL EntireSession)
{
        UINT32 nActionTimeout; // For restoring a parameter after this reset
        UINT32 nAllocationSize;

        // Validate the pointer
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

#ifdef _HTTP_DEBUGGING_
        if(pHTTPSession->pDebug)
        {
                pHTTPSession->pDebug("HTTPIntrnSessionReset",NULL,
                                0,"");
        }
#endif


        memset(pHTTPSession->HttpHeaders.HeadersIn.pParam,0x00,pHTTPSession->HttpHeaders.HeadersIn.nLength);
        pHTTPSession->HttpHeaders.HeadersIn.nLength = 0;


        // Reset the HTTP counters
        nActionTimeout = pHTTPSession->HttpCounters.nActionTimeout;
        memset(&pHTTPSession->HttpCounters,0x00,sizeof(HTTP_COUNTERS));
        pHTTPSession->HttpCounters.nActionStartTime = HTTPIntrnSessionGetUpTime();
        // Restore the parameter
        pHTTPSession->HttpCounters.nActionTimeout = nActionTimeout;

        // Reset the authentication flag
        pHTTPSession->HttpCredentials.Authentication = FALSE;


        if(EntireSession == TRUE) // Partial reset, clear only the incoming headers
        {
                memset(&pHTTPSession->HttpUrl,0,sizeof(HTTP_URL));
                nAllocationSize =  pHTTPSession->HttpHeaders.HeadersBuffer.nLength;
                // Reset the headers allocated memory
                memset(pHTTPSession->HttpHeaders.HeadersBuffer.pParam ,0x00,nAllocationSize);

                // Set default values in the session structure
                HTTPClientSetVerb((UINT32)(unsigned long)pHTTPSession,(HTTP_VERB)HTTP_CLIENT_DEFAULT_VERB);    // Default HTTP verb
                pHTTPSession->HttpUrl.nPort             = HTTP_CLIENT_DEFAULT_PORT;             // Default TCP port
                // Set the outgoing headers pointers
                memset(&pHTTPSession->HttpHeaders.HeadersIn,0,sizeof(HTTP_PARAM));
                memset(&pHTTPSession->HttpHeaders.HeadersOut,0,sizeof(HTTP_PARAM));

                pHTTPSession->HttpHeaders.HeadersOut.pParam = pHTTPSession->HttpHeaders.HeadersBuffer.pParam;
                // Set our state
                pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_INIT;

                memset(&pHTTPSession->HttpHeadersInfo,0,sizeof(HTTP_HEADERS_INFO));
                if(pHTTPSession->HttpConnection.HttpSocket != HTTP_INVALID_SOCKET)
                {
                        pHTTPSession->HttpHeadersInfo.Connection = TRUE;
                }
                memset(&pHTTPSession->HttpAuthHeader,0,sizeof(HTTP_AUTH_HEADER));
#ifdef HTTPC_PROXY
                memset(&pHTTPSession->HttpProxy,0,sizeof(HTTP_PROXY));
#endif

        }

        return HTTP_CLIENT_SUCCESS;

}


///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnHeadersReceive
// Purpose      : Receives the response header on the connection and parses it.
//                Performs any required authentication.
// Returns      : HTTP Status
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnHeadersReceive (P_HTTP_SESSION pHTTPSession,
                UINT32 nTimeout)        // [IN] Timeout for the operation

{

        UINT32          nRetCode;               // Function call return code
        UINT32          nCount  = 0;
        if(!pHTTPSession)
        {
                // Bad session pointer error
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        do
        {

                // Set the operation time out if was set by the caller
                if(nTimeout > 0)
                {
                        // 0 makes us use the default defined value
                        pHTTPSession->HttpCounters.nActionTimeout = HTTP_TIMEOUT(nTimeout);

                }

                // Reset the incoming headers
                if((nRetCode = HTTPIntrnSessionReset(pHTTPSession,FALSE)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // Get the server response
                if((nRetCode = HTTPIntrnGetRemoteHeaders(pHTTPSession)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // Set the session state
                pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HEADERS_RECIVED;

                // Parse the response headers
                if((nRetCode = HTTPIntrnHeadersParse(pHTTPSession)) != HTTP_CLIENT_SUCCESS)
                {
                        break;
                }

                // Set the session state
                pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HEADERS_PARSED;

                // Set the session stage upon seccess
                if(pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_OK)
                {
                        pHTTPSession->HttpState = pHTTPSession->HttpState | HTTP_CLIENT_STATE_HEADERS_OK;
                }
                // Handle 100 continue message
                if(pHTTPSession->HttpHeadersInfo.nHTTPStatus != HTTP_STATUS_CONTINUE)
                {
                        nCount++;
                }

#ifdef _HTTP_DEBUGGING_
                if(pHTTPSession->pDebug)
                {
                        if(pHTTPSession->HttpHeadersInfo.nHTTPStatus == HTTP_STATUS_CONTINUE)
                        {
                                pHTTPSession->pDebug("HTTPIntrnHeadersReceive",NULL,0,"100 Continue Header");
                        }
                }
#endif

        }while(nCount < 1);

        return nRetCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnGetTicks
// Purpose      : Like GetTickCount() (implemented with time.h)
// Gets         : void
// Returns      : System ticks
// Last updated : 01/09/200515/05/2005
// Author Name	: Eitan Michaelson
// Notes	    : Assuming 1000 ticks per sec, should be implemented by the OS
//
///////////////////////////////////////////////////////////////////////////////

UINT32 HTTPIntrnSessionGetUpTime(VOID)
{

        return GetUpTime();

}

///////////////////////////////////////////////////////////////////////////////
//
// Function     : HTTPIntrnSessionEvalTimeout
// Purpose      : Check if we have to break the operation and return a time out error
// Gets         : a pointer to the session structure
// Returns      : BOOL, True if we have to break
// Last updated : 01/09/2005
//
///////////////////////////////////////////////////////////////////////////////

BOOL HTTPIntrnSessionEvalTimeout(P_HTTP_SESSION pHTTPSession)
{
#ifdef	NO_TIMEOUTS
        return FALSE;
#else
        UINT32 nElapsedTime;    // Integer for calculating the elapsed time

        // Validate the session pointer
        if(!pHTTPSession)
        {
                return HTTP_CLIENT_ERROR_INVALID_HANDLE;
        }

        // Calculate the elapsed time since the last call
        nElapsedTime = HTTPIntrnSessionGetUpTime() - pHTTPSession->HttpCounters.nActionStartTime;
        // If the elapsed time is greater then the time out value we should return true
        if(nElapsedTime >= pHTTPSession->HttpCounters.nActionTimeout)
        {
                return TRUE;
        }

        return FALSE;
#endif
}
