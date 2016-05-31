#define __USE_XOPEN
#define _GNU_SOURCE

#include <time.h>
#include <string.h>
#include <stdio.h>
#include "request.h"
#include "safe_print.h"


#define until(x) while(!(x))

int find_next_char(char c, char *str, int start) {

    int result = start;

    until (str[result] == c || str[result] == '\0') {
        result++;
    }

    if (str[result] == '\0') {
        return -1;
    }

    return result;
}


int
find_next_clrf(char *str, int start) {

    int result = start;

    /* thinking in terms of 'while' was to complicated so we decided to define
     * our own control structure */
    until (str[result] == '\0' ||
            (str[result] == '\r' && str[result+1] == '\n')) {
        result++;
    }

    if (str[result] == '\0') {
        return -1;
    }

    return result;
}

http_status_t
parse_request(char *request, request_t *out) {

    /* Caution: the '\0' at the end of the first line is needed outside of this
     * function.  Don't change unless you know what you're doing. */
    int size = find_next_clrf(request, 0);
    request[size] = '\0';

    int ret;
    int result = parse_method_and_uri(request, out);
    if (result != HTTP_STATUS_OK) {
        return result;
    }


    out->range_start = 0;
    while (size != -1) {

        char *current_line = &request[size+2];
        size = find_next_clrf(request, size+2);
        request[size] = '\0';

        // TODO? check if fields are valid
        if (strncmp(current_line, "Range:", 6) == 0) {

            char *field_value = &current_line[6];
            ret = parse_range(field_value, out);
            if (ret == HTTP_STATUS_BAD_REQUEST) {
                return ret;
            }
            result = HTTP_STATUS_PARTIAL_CONTENT;
        }
        else if (strncmp(current_line, "If-Modified-Since:", 18) == 0) {

            char *field_value = &current_line[18];
            while (*field_value == ' ') {
                field_value++;
            }

            ret = parse_date(field_value, out);
            if (ret == HTTP_STATUS_BAD_REQUEST) {
                return ret;
            }
        }
    }

    return result;
}

http_status_t
parse_method_and_uri(char *first_line, request_t *out) {

    int offset;
    if (strncmp(first_line, "HEAD", 4) == 0) {
        offset = 5;
        out->method = HTTP_METHOD_HEAD;
    }
    else if (strncmp(first_line, "GET", 3) == 0) {
        offset = 4;
        out->method = HTTP_METHOD_GET;
    }
    else {
        return HTTP_STATUS_NOT_IMPLEMENTED;
    }

    // TODO inspect return value of malloc
    out->uri = (char *)malloc((MAX_SIZE_URI+1) * sizeof(char));

    int size = offset;
    do {
        size++;
    } while (size < MAX_SIZE_LINE && first_line[size] != ' ');

    if (size >= MAX_SIZE_URI) {
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
    else {
        memcpy(out->uri, &first_line[offset], size-offset);
        out->uri[size-offset] = '\0';
    }

    return HTTP_STATUS_OK;
}

http_status_t
parse_range(char *field, request_t *out) {

    // TODO? allow garbage

    int begin = find_next_char('=', field, 0);
    int end   = find_next_char('-', field, begin);

    field[end] = '\0';
    int range_begin;

    if (sscanf(&field[begin+1], "%d", &range_begin) != 1) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    out->range_start = range_begin;
    return HTTP_STATUS_PARTIAL_CONTENT;
}

http_status_t
parse_date(char *field, request_t *out) {

    struct tm timestruct;

    if (strptime(field, "%a, %d %b %Y %H:%M:%S GMT", &timestruct) != NULL) {
        out->modified_since = timegm(&timestruct);
    }
    else {
        return HTTP_STATUS_BAD_REQUEST;
    }

    return HTTP_STATUS_OK;
}
