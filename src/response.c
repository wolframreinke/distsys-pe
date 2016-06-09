#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "content.h"
#include "socket_io.h"
#include "safe_print.h"

#include "response.h"

#define FIELD_ACCEPT_RANGES "Accept-Ranges: bytes\r\n"
#define FIELD_CONNECTION    "Connection: Close\r\n"
#define FIELD_SERVER        "Server: TinyWeb\r\n"

#define IS_EXECUTABLE(mode) (S_ISREG(mode) && (S_IXOTH & (mode)))
#define IS_DIRECTORY(mode)  (S_ISDIR(mode) && ((S_IXOTH || S_IROTH) & (mode)))
#define IS_READABLE(mode)   (S_ISREG(mode) && (S_IROTH & (mode)))

extern char **environ;

/*!
 * \fn generate_response_header
 * \brief Generates the response header (response_t) for the given request
 *
 * \param filename  The filename of the requested resource (the file path
 *                  including the root dir, not just the requested URI)
 * \param status    The HTTP status to send.  This is not necessarily the status
 *                  that gets actually sent.  If an error occurs during the
 *                  execution of this function, an error status might be used.
 * \param req       The request for which the server response should be
 *                  generated.
 * \param out       The response_t to which the result is written.  Not all
 *                  fields are fully initialized if they are not necessary for
 *                  the given status code or request method.
 */
void
generate_response_header(char *filename, http_status_t status, request_t *req,
        response_t *out) {

    out->content_location = req->uri;
    out->date             = time(NULL);
    out->status           = status;

    // content-related fields are only send with status OK and PARTIAL_CONTENT
    if (status == HTTP_STATUS_OK || status == HTTP_STATUS_PARTIAL_CONTENT) {

        struct stat file_stats;
        if (stat(filename, &file_stats) == -1) {
            out->status = HTTP_STATUS_NOT_FOUND;
        }
        else if (IS_DIRECTORY(file_stats.st_mode)) {
            out->status = HTTP_STATUS_MOVED_PERMANENTLY;
        }
        else if (!IS_READABLE(file_stats.st_mode)) {
            out->status = HTTP_STATUS_FORBIDDEN;
        }
        else if (req->range_start >= file_stats.st_size ||
                 req->range_start  < 0) {
            out->status = HTTP_STATUS_RANGE_NOT_SATISFIABLE;
        }
        else {
            out->last_modified       = file_stats.st_mtime;
            out->content_range.begin = req->range_start;

            if (req->is_cgi) {
                if (!IS_EXECUTABLE(file_stats.st_mode)) {
                    out->status = HTTP_STATUS_FORBIDDEN;
                }
                out->is_cgi = 1;
            }
            else {
                out->content_range.total = file_stats.st_size;
                out->content_length      = file_stats.st_size- req->range_start;
                out->content_type        = get_http_content_type(filename);
                out->is_cgi              = 0;
            }

            if (out->last_modified <= req->modified_since) {
                out->status = HTTP_STATUS_NOT_MODIFIED;
            }
        }
    }
}


/*!
 * \fn send_date
 * \brief Formats and sends the given timestamp under the given name.
 *
 * (The parameter "buf" is only included because it is more convenient to reuse
 * the buffer in the public "send_response" function instead of allocating a new
 * buffer in this function).
 *
 * \param sd    The socket descriptor of the client.  The formatted date is
 *              sent through this descriptor.
 * \param date  The timestamp to format and send.
 * \param name  The name of the HTTP response fields which shall be used, e.g.
 *              "Date".
 * \param buf   A buffer which can be used to write the response line, should
 *              have length MAX_SIZE_LINE
 *
 * \return Either 0 when the function executed successfully, or a value smaller
 *         than 0 in case of an error.
 */
int
send_date(int sd, const time_t *date, char *name, char *buf) {

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


int
send_response(int sd, const char *filename, response_t *res) {

    int total_count = 0;


    char buf[MAX_SIZE_LINE];
    int cnt = sprintf(&buf[0], "HTTP/1.1 %d %s\r\n",
            http_status_list[res->status].code,
            http_status_list[res->status].text);

    if (cnt < 0) {
        return -1;
    }
    total_count += cnt;

    if ((cnt = write_to_socket(sd, buf, cnt, 0)) < 0) {
        return -1;
    }
    total_count += cnt;

    if (send_date(sd, &res->date, "Date", (char *)buf) < 0) {
        return -1;
    }

    if ((cnt = write_to_socket(sd, FIELD_SERVER, sizeof(FIELD_SERVER)-1, 0)) < 0) {
        return -1;
    }
    total_count += cnt;

    if (res->status == HTTP_STATUS_OK ||
            res->status == HTTP_STATUS_PARTIAL_CONTENT ||
            res->status == HTTP_STATUS_NOT_MODIFIED) {

        if (send_date(sd, &res->last_modified, "Last-Modified",
                    (char *)buf) < 0) {
            return -1;
        }


        if ((cnt = write_to_socket(sd, FIELD_ACCEPT_RANGES,
                    sizeof(FIELD_ACCEPT_RANGES)-1, 0)) < 0) {
            return -1;
        }
        total_count += cnt;

        if ((cnt = write_to_socket(sd, FIELD_CONNECTION,
                    sizeof(FIELD_CONNECTION)-1, 0)) < 0) {
            return -1;
        }
        total_count += cnt;


        if (res->is_cgi) {

            if (res->status != HTTP_STATUS_NOT_MODIFIED &&
                    res->method == HTTP_METHOD_GET) {

                int pid;
                int fd_pipe[2];

                if (pipe(fd_pipe) == -1) {
                    perror("ERROR: pipe()");
                    return -1;
                }

                if ((pid = fork()) < 0) {
                    perror("ERROR: fork() before execle");
                    return -1;
                }
                else if (pid > 0) {

                    close(fd_pipe[1]);
                    char buf_cgi[MAX_SIZE_BUFFER_CGI];

                    do {
                        // TODO Fix read
                        if (waitpid(pid, NULL, WNOHANG) != -1) {
                            cnt = read(fd_pipe[0], &buf_cgi[0], MAX_SIZE_BUFFER_CGI);
                        }
                        if (cnt < 0) {
                            if (errno != EINTR) {
                                perror("ERROR: read() from pipe");
                                return -1;
                            }
                        }
                        else {
                            if (write(sd, buf_cgi, cnt) == -1) {
                                perror("ERROR: write() to socket");
                                return -1;
                            }
                            total_count += cnt;
                        }

                    } while (cnt > 0);
                    close(fd_pipe[0]);
                }
                else {
                    close(fd_pipe[0]);
                    dup2(fd_pipe[1], 1);
                    dup2(fd_pipe[1], 2);
                    execle(filename, filename, (char *)NULL, environ);
                    perror("ERROR: execle() on CGI script");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else {

            if ((cnt = sprintf(buf, "Content-Type: %s\r\n",
                           get_http_content_type_str(res->content_type))) < 0) {

                return -1;
            }

            if ((cnt = write_to_socket(sd, buf, cnt, 0)) < 0) {
                return -1;
            }
            total_count += cnt;

            if ((cnt = sprintf(buf, "Content-Length: %u\r\n", res->content_length))
                    < 0) {
                return -1;
            }

            if ((cnt = write_to_socket(sd, buf, cnt, 0)) < 0) {
                return -1;
            }
            total_count += cnt;


            if ((cnt = sprintf(buf, "Content-Range: bytes %d-%d/%d\r\n\r\n",
                            res->content_range.begin,
                            res->content_range.total - 1,
                            res->content_range.total)) < 0) {
                return -1;
            }
            if ((cnt = write_to_socket(sd, buf, cnt, 0)) < 0) {
                return -1;
            }
            total_count += cnt;


            if (res->status != HTTP_STATUS_NOT_MODIFIED &&
                    res->method == HTTP_METHOD_GET) {

                int fd;
                if ((fd = open(filename, O_RDONLY)) < 0) {
                    perror("ERROR: open()");
                    return -1;
                }
                off_t offset = res->content_range.begin;

                if (sendfile(sd, fd, &offset, res->content_length) < 0) {
                    perror("ERROR: sendfile()");
                    return -1;
                }
                total_count += res->content_length;
            }
        }

    }
    else if (res->status == HTTP_STATUS_MOVED_PERMANENTLY) {

        if ((cnt = sprintf(buf, "Location: %s/\r\n\r\n",
                        res->content_location)) < 0) {
            return -1;
        }
        if ((cnt = write_to_socket(sd, buf, cnt, 0)) < 0) {
            return -1;
        }
        total_count += cnt;
    }

    return total_count;
}

void
send_static_500(int sd) {

    char buf[MAX_SIZE_LINE];
    int cnt = sprintf(&buf[0], "HTTP/1.1 500 Internal Server Error\r\n");
    if (cnt < 0) {
        return;
    }

    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        return;
    }

    char timebuf[32];
    time_t now = time(NULL);
    struct tm *timestruct = gmtime(&now);
    // TODO might be wrong
    strftime(timebuf, 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", &timestruct[0]);
    timebuf[31] = '\0';

    if ((cnt = sprintf(buf, "Date: %s", timebuf)) < 0) {
        return;
    }
    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        return;
    }
    if (write_to_socket(sd, FIELD_SERVER, sizeof(FIELD_SERVER), 0) < 0) {
        return;
    }
    if (write_to_socket(sd, "\r\n", 2, 0) < 0) {
        return;
    }
}
