/*! \file       http.c
 *  \author     Ralf Reutemann
 *  \brief      Provides http information.
 *
 *  This file defines http information about the availability of http methods
 *  and http information codes.
 */
/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "tinyweb.h"
#include "http.h"

/*! \brief The HTTP methods and their implementation status.*/
http_method_entry_t http_method_list[] = {
    { "GET",         HTTP_METHOD_GET             },
    { "HEAD",        HTTP_METHOD_HEAD            },
    { "TEST",        HTTP_METHOD_NOT_IMPLEMENTED },
    { "ECHO",        HTTP_METHOD_NOT_IMPLEMENTED },
    { "OPTIONS",     HTTP_METHOD_NOT_IMPLEMENTED },
    { "POST",        HTTP_METHOD_NOT_IMPLEMENTED },
    { "PUT",         HTTP_METHOD_NOT_IMPLEMENTED },
    { "DELETE",      HTTP_METHOD_NOT_IMPLEMENTED },
    { "TRACE",       HTTP_METHOD_NOT_IMPLEMENTED },
    { "CONNECT",     HTTP_METHOD_NOT_IMPLEMENTED },
    { NULL,          HTTP_METHOD_NOT_IMPLEMENTED }
};

/*! \brief The HTTP infomration codes and their explanation.*/
http_status_entry_t http_status_list[] = {
    { 200, "OK"                              },  /* HTTP_STATUS_OK                    */
    { 206, "Partial Content"                 },  /* HTTP_STATUS_PARTIAL_CONTENT       */
    { 301, "Moved Permanently"               },  /* HTTP_STATUS_MOVED_PERMANENTLY     */
    { 304, "Not Modified"                    },  /* HTTP_STATUS_NOT_MODIFIED          */
    { 400, "Bad Request"                     },  /* HTTP_STATUS_BAD_REQUEST           */
    { 403, "Forbidden"                       },  /* HTTP_STATUS_FORBIDDEN             */
    { 404, "Not Found"                       },  /* HTTP_STATUS_NOT_FOUND             */
    { 416, "Requested Range Not Satisfiable" },  /* HTTP_STATUS_RANGE_NOT_SATISFIABLE */
    { 500, "Internal Server Error"           },  /* HTTP_STATUS_INTERNAL_SERVER_ERROR */
    { 501, "Not Implemented"                 }   /* HTTP_STATUS_NOT_IMPLEMENTED       */
};

