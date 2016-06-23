/*! \file       request.c
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Parsing HTTP requests.
 *
 *  See request.h for API documentation.
 */

#define __USE_XOPEN
#define _GNU_SOURCE

#include <time.h>
#include <string.h>
#include <stdio.h>
#include "request.h"
#include "safe_print.h"


#define until(x) while(!(x))

// helper functions, defined below parse_request
static http_status_t parse_method_and_uri(char *first_line, request_t *out);
static http_status_t parse_range(char *field, request_t *out);
static http_status_t parse_date(char *field, request_t *out);


/* --------------------------------------------------------------------------
 *  parse_request(strrequest, request)
 * -------------------------------------------------------------------------- */
/*! \brief Parses a HTTP request
 *
 *  This function is only concerned with the structural analysis of the given
 *  HTTP request, and will as such return error codes if the request is
 *  syntactically invalid.  It does not check if the request is semantically
 *  valid.
 *
 *  \param strrequest  The null-terminated request string.  This input string is
 *                     modified, so a safety copy should be created if one needs
 *                     to preserve the original input string.  In particular,
 *                     every "\r\n" in the string is replaced with "\0\n".
 *  \param request     The request_t pointer to which the result is written.
 *
 *  \return  A HTTP status code.  If the request could not be parsed correctly,
 *           a code different from HTTP_STATUS_OK is returned.  This error code
 *           can be used directly to generate a HTTP response and all fields in
 *           the output request_t will be set appropriately.  If the return
 *           value is HTTP_STATUS_OK, this does not mean that the request is
 *           completely valid.  For instance, this function does not ensure that
 *           the requested file exists and is readable.
 */
http_status_t
parse_request(char *strrequest, request_t *request) {

    int result = parse_method_and_uri(strrequest, request);
    if (result != HTTP_STATUS_OK) {
        return result;
    }

    /* Caution: the '\0' at the end of the first line is needed outside of this
     * function.  Don't change unless you know what you're doing. */
    char *rest = strstr(strrequest, "\r\n");
    *rest = '\0';



    /* sets the default value for the requested Content-Range.  This default
     * value might be overwritten below */
    request->range_start = 0;

    while (rest != NULL) {

        int ret;
        char *current_line = rest + 2;

        if (strncmp(current_line, "Range:", 6) == 0) {

            char *field_value = &current_line[6];
            ret = parse_range(field_value, request);
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

            ret = parse_date(field_value, request);
            if (ret == HTTP_STATUS_BAD_REQUEST) {
                return ret;
            }
        }

        rest = strstr(rest + 2, "\r\n");
    }

    return result;
}


/* ======================== PRIVATE HELPER FUNCTIONS ======================== */

/* --------------------------------------------------------------------------
 *  parse_method_and_uri(first_line, out)
 * -------------------------------------------------------------------------- */
/*! \brief Determines the HTTP method and the requested URI.
 *
 *  Both values are written to the given request_t pointer.
 *
 *  \param first_line    The first line of the HTTP request (method and URI are
 *                       parsed from the first line)
 *  \param out           The request_t pointer to which the result is written.
 *
 *  \return  A HTTP status code.  If the return value is not HTTP_STATUS_OK, an
 *           error occurred.
 */
static http_status_t
parse_method_and_uri(char *first_line, request_t *out) {

    char *uri;
    if (strncmp(first_line, "HEAD", 4) == 0) {
        out->method = HTTP_METHOD_HEAD;
        uri = first_line + 5;   /* uri points to the char after "HEAD " */
    }
    else if (strncmp(first_line, "GET", 3) == 0) {
        out->method = HTTP_METHOD_GET;
        uri = first_line + 4;   /* uri points to the char after "GET " */
    }
    else {
        return HTTP_STATUS_NOT_IMPLEMENTED;
    }

    /* allocates MAX_SIZE_URI bytes for the URI. The terminating null-byte will
     * be written to this buffer too */
    out->uri = (char *)malloc(MAX_SIZE_URI * sizeof(char));
    if (out->uri == NULL) {
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    /* looks for the next whitespace character, which should occur directly
     * after the URI.  If it cannot be found, or, the URI is too large
     * (difference between start of URI and end of URI + trailing '\0' byte */
    char *nextspace = strchr(uri, ' ');
    if (nextspace == NULL || nextspace-uri+1 > MAX_SIZE_URI) {
        return HTTP_STATUS_BAD_REQUEST;
    }
    else {
        /* temporarily null-terminates the URI in the request */
        *nextspace = '\0';
        strcpy(out->uri, uri);

        /* this is necessary since the first line of the request will be written
         * to the log file, and the null-byte would interfere with thatj */
        *nextspace = ' ';

        /* the requested file is a CGI script if the URI starts with /cgi-bin */
        out->is_cgi = (strncmp(out->uri, "/cgi-bin", 8) == 0);
    }

    return HTTP_STATUS_OK;
}


/* --------------------------------------------------------------------------
 *  parse_range(field, out)
 * -------------------------------------------------------------------------- */
/*! \brief Parses the value of a Content-Range field
 *
 *  The Content-Range field value must have the format bytes=<begin>-, that is
 *  it must only contain a start index but not an end index.  If it contains an
 *  end index, only the start index is parsed and no error occurs.
 *
 *  \param field  The Content-Range field value
 *  \param out    The request_t pointer to which the result is written.
 *
 *  \return  A HTTP status code.  If the return value is not HTTP_STATUS_OK, an
 *           error occurred.
 */
static http_status_t
parse_range(char *field, request_t *out) {

    /* This function does not check if the request uses the exact format
     * 'bytes=<begin>-', instead it just parses the integer directly after the
     * first '='.
     */

    char *begin = strchr(field, '=');
    if (begin == NULL) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    int range_begin;
    if (sscanf(begin + 1, "%d", &range_begin) != 1) {
        return HTTP_STATUS_BAD_REQUEST;
    }

    out->range_start = range_begin;
    return HTTP_STATUS_PARTIAL_CONTENT;
}


/* --------------------------------------------------------------------------
 *  parse_date(field, out)
 * -------------------------------------------------------------------------- */
/*! \brief Parses the value a date field (If-Modified-Since and Date)
 *
 *  \param field  The Date or If-Modified-Since field value
 *  \param out    The request_t pointer to which the result is written.
 *
 *  \return  A HTTP status code.  If the return value is not HTTP_STATUS_OK, an
 *           error occurred.
 */
static http_status_t
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
