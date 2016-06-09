#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "content.h"
#include "http.h"
#include "request.h"

/*!
 * \brief The start and end index of the Content-Range fields of a HTTP request
 */
typedef struct {
    int begin; /*!< the start index of the Content-Range field */
    int total; /*!< the end index of the Content-Range field */
} content_range_t;


/*!
 * \brief Information about a HTTP response
 *
 * This struct does not contain some fields, since their values are always the
 * same.  Here are these fields and their values:
 *
 *      Connection     "Close"
 *      Server         "Tinyweb"
 *      Accept-Range   "bytes"
 */
typedef struct {

    /*! The status of the response */
    http_status_t status;

    /*! The method of the request associated with this response */
    http_method_t method;

    /*! The date and time when the response is generated */
    time_t date;

    /*! The time the requested file was last modified */
    time_t last_modified;

    /*! The length of the requested file in bytes */
    unsigned int content_length;

    /*! The content type of the requested type */
    http_content_type_t content_type;

    /*! The URI of the requested file (same as in HTTP request) */
    char *content_location;

    /*! File range to send */
    content_range_t content_range;

    /*! If the requested file is a CGI script */
    int is_cgi;

} response_t;

void
generate_response_header(char *path, http_status_t status, request_t *req,
        response_t *out);

int
send_response(int sd, const char *filename, response_t *res);

void
send_static_500(int sd);

#endif // _RESPONSE_H_
