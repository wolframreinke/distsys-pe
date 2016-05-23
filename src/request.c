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

void
parse_request(char *request, size_t len, request_t *out) {

    int size = find_next_clrf(request, 0);
    request[size] = '\0';
    parse_method_and_uri(request, size, out);


    while (size != -1) {

        char *current_line = &request[size+2];
        size = find_next_clrf(request, size+2);
        request[size] = '\0';

        // TODO? check if fields are valid
        if (strncmp(current_line, "Range:", 6) == 0) {

            char *field_value = &current_line[6];
            parse_range(field_value, 0, out);
        }
    }

    return;
}

void
parse_method_and_uri(char *first_line, size_t len, request_t *out) {

    if (strncmp(first_line, "HEAD", 4) == 0) {

    }
    else if (strncmp(first_line, "GET", 3) == 0) {

        out->method = HTTP_METHOD_GET;
        out->uri = (char *)malloc((MAX_SIZE_URI+1) * sizeof(char));

        int size = 4;
        do {
            size++;
        } while (size < MAX_SIZE_LINE && first_line[size] != ' ');

        if (size >= MAX_SIZE_URI) {
            // TODO ERROR
        }
        else {
            memcpy(out->uri, &first_line[4], size-4);
            out->uri[size] = '\0';
        }
    }
    else {
        // TODO handle error
    }
}

void
parse_range(char *field, size_t len, request_t *out) {

    // TODO? allow garbage

    int begin = find_next_char('=', field, 0);
    int end   = find_next_char('-', field, begin);

    field[end] = '\0';
    int range_begin;

    if (sscanf(&field[begin+1], "%d", &range_begin) != 1) {
        // TODO handle error
    }

    out->range_start = range_begin;
}
