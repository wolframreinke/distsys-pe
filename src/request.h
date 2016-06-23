/*! \file       request.h
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Parsing HTTP requests.
 *
 *  This module contains HTTP requests (request_t), and the function
 *  parse_request(), which parses a HTTP request from a string.
 */

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <time.h>
#include "http.h"

#define MAX_SIZE_REQUEST    2048
#define MAX_SIZE_URI         255
#define MAX_SIZE_LINE        512
#define MAX_SIZE_BUFFER_CGI 2048

#define TRUE  1
#define FALSE 0

/*!
 *  \brief The contents of a HTTP GET or HEAD request
 */
typedef struct {

    http_method_t method;  /*!< The HTTP request method, only GET and HEAD are
                                supported */
    char *uri;             /*!< The requested URI */
    int range_start;       /*!< The first component of the Content-Range field,
                                the second component is always EOF */
    time_t modified_since; /*!< The value of the If-Modified-Since field, if
                                sent */
    int is_cgi;            /*!< If the URI starts with /cgi-bin (that is, we
                                need to execute a CGI script */

} request_t;

http_status_t
parse_request(char *request, request_t *out);

#endif // _REQUEST_H_
