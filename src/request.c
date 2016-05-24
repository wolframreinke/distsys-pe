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

int
parse_request(char *request, size_t len, request_t *out) {

    int size = find_next_clrf(request, 0);
    request[size] = '\0';

    int ret;
    int result = parse_method_and_uri(request, size, out);
    if (result != REQUEST_NORMAL) {
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
            ret = parse_range(field_value, 0, out);
            if (ret == REQUEST_COULD_NOT_PARSE) {
                return ret;
            }
            result = REQUEST_PARTIAL;
        }
        else if (strncmp(current_line, "If-Modified-Since:", 18) == 0) {

            char *field_value = &current_line[18];
            while (*field_value == ' ') {
                field_value++;
            }

            ret = parse_date(field_value, 0, out);
            if (ret == REQUEST_COULD_NOT_PARSE) {
                return ret;
            }
        }
    }

    return result;
}

int
parse_method_and_uri(char *first_line, size_t len, request_t *out) {

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
        return REQUEST_UNSUPPORTED;
    }

    out->uri = (char *)malloc((MAX_SIZE_URI+1) * sizeof(char));

    int size = offset;
    do {
        size++;
    } while (size < MAX_SIZE_LINE && first_line[size] != ' ');

    if (size >= MAX_SIZE_URI) {
        return REQUEST_ERROR;
    }
    else {
        memcpy(out->uri, &first_line[offset], size-offset);
        out->uri[size-offset] = '\0';
    }

    return REQUEST_NORMAL;
}

int
parse_range(char *field, size_t len, request_t *out) {

    // TODO? allow garbage

    int begin = find_next_char('=', field, 0);
    int end   = find_next_char('-', field, begin);

    field[end] = '\0';
    int range_begin;

    if (sscanf(&field[begin+1], "%d", &range_begin) != 1) {
        return REQUEST_COULD_NOT_PARSE;
    }

    out->range_start = range_begin;
    return REQUEST_PARTIAL;
}

int
parse_date(char *field, size_t len, request_t *out) {

    struct tm timestruct;

    if (strptime(field, "%a, %d %b %Y %H:%M:%S GMT", &timestruct) != NULL) {
        out->modified_since = timegm(&timestruct);
    }
    else {
        return REQUEST_COULD_NOT_PARSE;
    }

    return REQUEST_NORMAL;
}
