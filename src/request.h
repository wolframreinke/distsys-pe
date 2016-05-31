#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <time.h>
#include "http.h"

#define MAX_SIZE_REQUEST    2048
#define MAX_SIZE_URI         255
#define MAX_SIZE_LINE        512


typedef struct {

    http_method_t method;
    char *uri;

    int    range_start;        // end is always end of file
    time_t modified_since;

} request_t;

http_status_t
parse_request(char *request, request_t *out);

http_status_t
parse_method_and_uri(char *first_line, request_t *out);

http_status_t
parse_range(char *field, request_t *out);

http_status_t
parse_date(char *field, request_t *out);

#endif // _REQUEST_H_
