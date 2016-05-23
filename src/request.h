
#include "http.h"

#define MAX_SIZE_REQUEST    2048
#define MAX_SIZE_URI         255
#define MAX_SIZE_LINE        512


typedef struct {

    http_method_t method;
    char *uri;

    int range_start;        // end is always end of file

} request_t;

void
parse_request(char *request, size_t len, request_t *out);

void
parse_method_and_uri(char *first_line, size_t len, request_t *out);

void
parse_range(char *field, size_t len, request_t *out);
