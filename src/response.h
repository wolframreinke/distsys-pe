#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "content.h"
#include "http.h"
#include "request.h"

typedef struct {

    int begin;
    int total;

} content_range_t;

typedef struct {

    http_status_t status;

    // Connection is always "Close"
    // Server is always the same
    // Accept-Range is always "bytes"

    http_method_t       method;
    time_t              date;
    time_t              last_modified;
    unsigned int        content_length;
    http_content_type_t content_type;
    char               *content_location;
    content_range_t     content_range;

} response_t;

void
generate_response_header(char *path, http_status_t status, request_t *req,
        response_t *out);

int
send_response(int sd, const char *filename, response_t *res);

void
send_static_500(int sd);

#endif // _RESPONSE_H_
