/*! \file       response.h
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Generating HTTP responses from requests and sending them.
 *
 *  This module defines HTTP response headers (response_t), the function
 *  generate_response_header(), which generates them from HTTP requests, and the
 *  function send_response(), which sends a full HTTP response to through a
 *  socket descriptor when given a response header.  Additionally, the function
 *  send_static_500() can be used to send a "500 - Internal Server Error"
 *  message to a client without the requirement for additional memory.
 */

#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "content.h"
#include "http.h"
#include "request.h"

/*!
 *  \brief The start and end index of the Content-Range fields of a HTTP request
 */
typedef struct {
    int begin; /*!< the start index of the Content-Range field */
    int total; /*!< the end index of the Content-Range field */
} content_range_t;


/*! \brief The contents of a HTTP response header.
 *
 *  This struct does not contain all possible header fields, but only the ones
 *  supported by tinyweb.  It also does not include some manditory attributes,
 *  as they always have the same value:
 *
 *  \code{.unparsed}
 *      Connection     "Close"
 *      Server         "Tinyweb"
 *      Accept-Range   "bytes"
 *  \endcode
 */
typedef struct {

    http_status_t status;             /*!< The status of the response */
    http_method_t method;             /*!< The method of the request associated
                                           with this response */
    time_t date;                      /*!< The date and time when the response
                                           is generated */
    time_t last_modified;             /*!< The time the requested file was last
                                           modified */
    size_t content_length;            /*!< The length of the requested file in
                                           bytes */
    http_content_type_t content_type; /*!< The content type of the requested
                                           type */
    char *content_location;           /*!< The URI of the requested file (same
                                           as in HTTP request) */
    content_range_t content_range;    /*!< File range to send */
    int is_cgi;                       /*!< whether the requested file is a CGI
                                           script */
} response_t;

void
generate_response_header(char *path, http_status_t status, request_t *req,
        response_t *out);

int
send_response(int sd, const char *filename, response_t *res);

void
send_static_500(int sd);

#endif // _RESPONSE_H_
