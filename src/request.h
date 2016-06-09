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
 * \brief Information about a HTTP request.
 */
typedef struct {

    /*! The HTTP request method, only GET and HEAD are supported */
    http_method_t method;

    /*! The requested URI */
    char *uri;

    /*! The first component of the Content-Range field, the second component is
     * always EOF */
    int range_start;

     /*!< The value of the If-Modified-Since field, if sent */
    time_t modified_since;

    /*!< If the URI starts with /cgi-bin (that is, we need to execute a CGI
     * script */
    int is_cgi;

} request_t;

http_status_t
parse_request(char *request, request_t *out);

#endif // _REQUEST_H_
