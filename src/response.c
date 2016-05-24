#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>

#include "content.h"
#include "socket_io.h"
#include "safe_print.h"

#include "response.h"

#define FIELD_ACCEPT_RANGES "Accept-Ranges: bytes\r\n"
#define FIELD_CONNECTION    "Connection: Close\r\n"
#define FIELD_SERVER        "Server: TinyWeb\r\n"

#define IS_READABLE(mode)   (S_ISREG(mode) && (S_IROTH & (mode)))


void
generate_response_header(char *filename, http_status_t status, request_t *req,
        response_t *out) {

    out->content_location = req->uri;
    out->date             = time(NULL);
    out->status           = status;

    if (status == HTTP_STATUS_OK || status == HTTP_STATUS_PARTIAL_CONTENT) {

        struct stat file_stats;
        if (stat(filename, &file_stats) == -1) {
            out->status = HTTP_STATUS_NOT_FOUND;
        }
        else if (!IS_READABLE(file_stats.st_mode)) {
            out->status = HTTP_STATUS_FORBIDDEN;
        }
        else if (req->range_start >= file_stats.st_size) {
            out->status = HTTP_STATUS_RANGE_NOT_SATISFIABLE;
        }
        else {
            out->content_range.begin = req->range_start;
            out->content_range.total = file_stats.st_size;
            out->content_length      = file_stats.st_size - req->range_start;
            out->last_modified       = file_stats.st_mtime;
            out->content_type        = get_http_content_type(filename);

            if (out->last_modified <= req->modified_since) {
                out->status = HTTP_STATUS_NOT_MODIFIED;
            }
        }
    }
}


int
send_date(int sd, const time_t *date, char *name,
        char *buf) {

    int cnt;
    char timebuf[32];
    struct tm *timestruct = gmtime(date);
    // TODO might be wrong
    strftime(timebuf, 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", &timestruct[0]);
    timebuf[31] = '\0';

    if ((cnt = sprintf(buf, "%s: %s", name, timebuf)) < 0) {
        return -1;
    }
    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        return -2;
    }

    return 0;
}


void
send_response(int sd, const char *filename, response_t *res) {

    // HTTP/1.1 <nummer> <text>

    //char *status_text;

    char buf[MAX_SIZE_LINE];
    int cnt = sprintf(&buf[0], "HTTP/1.1 %d %s\r\n",
            http_status_list[res->status].code,
            http_status_list[res->status].text);

    if (cnt < 0) {
        // TODO handle error
        return;
    }

    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        // TODO handle error
        return;
    }

    if (send_date(sd, &res->date, "Date", (char *)buf) < 0) {
        // TODO handle error
        return;
    }

    if (write_to_socket(sd, FIELD_SERVER, sizeof(FIELD_SERVER), 0) < 0) {
        //TODO handle error
        return;
    }

    if (res->status == HTTP_STATUS_OK ||
            res->status == HTTP_STATUS_PARTIAL_CONTENT ||
            res->status == HTTP_STATUS_NOT_MODIFIED) {

        if (write_to_socket(sd, FIELD_ACCEPT_RANGES,
                    sizeof(FIELD_ACCEPT_RANGES), 0) < 0) {
            //TODO handle error
            return;
        }
        if (write_to_socket(sd, FIELD_CONNECTION,
                    sizeof(FIELD_CONNECTION), 0) < 0) {
            //TODO handle error
            return;
        }

        // TODO content length ist bei partial content noch falsch

        if (send_date(sd, &res->last_modified, "Last-Modified",
                    (char *)buf) < 0) {
            // TODO handle error
            return;
        }

        if ((cnt = sprintf(buf, "Content-Type: %s\r\n",
                        get_http_content_type_str(res->content_type))) < 0) {

            // TODO handle error
            return;
        }

        if (write_to_socket(sd, buf, cnt, 0) < 0) {
            //TODO handle error
            return;
        }

        if ((cnt = sprintf(buf, "Content-Length: %u\r\n", res->content_length))
                < 0) {
            // TODO handle error
            return;
        }

        if (write_to_socket(sd, buf, cnt, 0) < 0) {
            //TODO handle error
            return;
        }

        if ((cnt = sprintf(buf, "Content-Range: bytes %d-%d/%d\r\n\r\n",
                        res->content_range.begin,
                        res->content_range.total - 1,
                        res->content_range.total)) < 0) {
            // TODO handle error
            return;
        }

        if (write_to_socket(sd, buf, cnt, 0) < 0) {
            //TODO handle error
            return;
        }

        if (res->status != HTTP_STATUS_NOT_MODIFIED &&
                res->method == HTTP_METHOD_GET) {

            int fd = open(filename, O_RDONLY);
            off_t offset = res->content_range.begin;

            sendfile(sd, fd, &offset, res->content_length);

        }
    }
    else if (res->status == HTTP_STATUS_MOVED_PERMANENTLY) {

    }
    else if (res->status == HTTP_STATUS_NOT_MODIFIED) {

    }
}
