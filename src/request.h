#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <time.h>
#include "http.h"

#define MAX_SIZE_REQUEST    2048
#define MAX_SIZE_URI         255
#define MAX_SIZE_LINE        512

#define REQUEST_PARTIAL          1
#define REQUEST_NORMAL           0
#define REQUEST_ERROR           -1
#define REQUEST_COULD_NOT_PARSE -2
#define REQUEST_UNSUPPORTED     -3


typedef struct {

    http_method_t method;
    char *uri;

    int    range_start;        // end is always end of file
    time_t modified_since;

} request_t;

int
parse_request(char *request, size_t len, request_t *out);

int
parse_method_and_uri(char *first_line, size_t len, request_t *out);

int
parse_range(char *field, size_t len, request_t *out);

int
parse_date(char *field, size_t len, request_t *out);

#endif // _REQUEST_H_
