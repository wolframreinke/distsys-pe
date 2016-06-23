/*! \file       response.c
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Functions to generate and send HTTP responses
 *
 *  See response.h for API documentation.
 */

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

/* used to ensure that CGI script child processes inherit env vars. */
extern char **environ;

/* helper functions, defined at the bottom of the file */
static int send_cgi_output(int sd_client, const char *filename);
static int send_date(int sd, const time_t *date, char *name, char *buf);

/* --------------------------------------------------------------------------
 *  generate_response_header(filename, status, req, out)
 * -------------------------------------------------------------------------- */
/*! \brief Generates the response header (response_t) for the given request
 *
 *  \param filename  The filename of the requested resource (the file path
 *                   including the root dir, not just the requested URI)
 *  \param status    The HTTP status to send.  This is not necessarily the
 *                   status that gets actually sent.  If an error occurs during
 *                   the execution of this function, a different status might be
 *                   used.
 *  \param req       The request for which the server response should be
 *                   generated.
 *  \param out       The response_t to which the result is written.  Not all
 *                   fields are fully initialized if they are not necessary for
 *                   the given status code or request method.
 */
void
generate_response_header(char *filename, http_status_t status, request_t *req,
        response_t *out) {

    out->content_location = req->uri;
    out->date             = time(NULL);
    out->status           = status;

    /* content-related fields are only send for status OK and PARTIAL_CONTENT */
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
                out->is_cgi = 1;
                if (!IS_EXECUTABLE(file_stats.st_mode)) {
                    out->status = HTTP_STATUS_FORBIDDEN;
                }
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


/* --------------------------------------------------------------------------
 *  send_response(sd_client, filename, res)
 * -------------------------------------------------------------------------- */
/*! \brief Writes the given HTTP response to the specified socket descriptor
 *
 *  If any of the steps of the function fails, partial output might be written
 *  to the socket descriptor.  If the requested file is a CGI script, the script
 *  will be executed in a new child process.
 *
 *  \param sd_client  The socket descriptor to which the HTTP response shall be
 *                    written.
 *  \param filename   The path to the requested file, either as an absolute path
 *                    or as a relative path from the current working directory
 *                    (that is, including the root directory of tinyweb).
 *  \param res        The HTTP response header data.
 *
 *  \return           On success, the number of bytes written to the socket
 *                    descriptor, on error -1.
 */
int
send_response(int sd_client, const char *filename, response_t *res) {

    int bytes_sent = 0, cnt;
    char buf[MAX_SIZE_LINE];

    /* Local macro to remove some boilerplate.  This macro is undef'd at the end
     * of the function */
    #define SEND_TO_CLIENT(buffer, length)                                   \
        {                                                                    \
            if ((cnt = write_to_socket(sd_client, buffer, length, 0)) < 0) { \
                return -1;                                                   \
            }                                                                \
            bytes_sent += cnt;                                               \
        }

    cnt = sprintf(buf, "HTTP/1.1 %d %s\r\n",
            http_status_list[res->status].code,
            http_status_list[res->status].text);

    if (cnt < 0) { return -1; }
    bytes_sent += cnt;
    SEND_TO_CLIENT(buf, cnt);

    if (send_date(sd_client, &res->date, "Date", (char *)buf) < 0) {
        return -1;
    }

    SEND_TO_CLIENT(FIELD_SERVER, sizeof(FIELD_SERVER)-1);

    if (res->status == HTTP_STATUS_OK ||
            res->status == HTTP_STATUS_PARTIAL_CONTENT ||
            res->status == HTTP_STATUS_NOT_MODIFIED) {

        if (send_date(sd_client, &res->last_modified, "Last-Modified",
                    (char *)buf) < 0) {
            return -1;
        }


        SEND_TO_CLIENT(FIELD_ACCEPT_RANGES, sizeof(FIELD_ACCEPT_RANGES)-1);
        SEND_TO_CLIENT(FIELD_CONNECTION, sizeof(FIELD_CONNECTION)-1);

        if (res->is_cgi) {

            if (res->status != HTTP_STATUS_NOT_MODIFIED &&
                    res->method == HTTP_METHOD_GET) {
                if ((cnt = send_cgi_output(sd_client, filename)) < 0) {
                    return -1;
                }
                bytes_sent += cnt;
            }
        }
        else {

            if ((cnt = sprintf(buf, "Content-Type: %s\r\n",
                           get_http_content_type_str(res->content_type))) < 0) {

                return -1;
            }
            SEND_TO_CLIENT(buf, cnt);

            if ((cnt = sprintf(buf, "Content-Length: %zd\r\n",
                            res->content_length)) < 0) {
                return -1;
            }
            SEND_TO_CLIENT(buf, cnt);


            if ((cnt = sprintf(buf, "Content-Range: bytes %d-%d/%d\r\n\r\n",
                            res->content_range.begin,
                            res->content_range.total - 1,
                            res->content_range.total)) < 0) {
                return -1;
            }
            SEND_TO_CLIENT(buf, cnt);


            if (res->status != HTTP_STATUS_NOT_MODIFIED &&
                    res->method == HTTP_METHOD_GET) {

                int fd;
                if ((fd = open(filename, O_RDONLY)) < 0) {
                    perror("ERROR: open()");
                    return -1;
                }
                off_t offset = res->content_range.begin;

                if (sendfile(sd_client, fd, &offset, res->content_length) < 0) {
                    perror("ERROR: sendfile()");
                    return -1;
                }
                bytes_sent += res->content_length;
            }
        }

    }
    else if (res->status == HTTP_STATUS_MOVED_PERMANENTLY) {

        if ((cnt = sprintf(buf, "Location: %s/\r\n\r\n",
                        res->content_location)) < 0) {
            return -1;
        }
        SEND_TO_CLIENT(buf, cnt);
    }

    return bytes_sent;

    #undef SEND_TO_CLIENT
}

/* --------------------------------------------------------------------------
 *  send_static_500(sd)
 * -------------------------------------------------------------------------- */
/*! \brief Writes a HTTP response with status 500 to the given socket descriptor
 *
 *  This function does not allocate additional memory, therefore it is possible
 *  to use this function in cases where not enough memory can be allocated to
 *  reply to the client normally.
 *
 *  The generated response does not include a body.
 *
 *  \param sd  The socket descriptor to which the HTTP response shall be
 *             written.
 */
void
send_static_500(int sd) {

    /* local stack allocations like these should be fine (?) */
    char buf[MAX_SIZE_LINE], timebuf[32];
    int cnt;
    time_t now = time(NULL);
    struct tm *timestruct = gmtime(&now);

    if ((cnt = sprintf(buf, "HTTP/1.1 500 Internal Server Error\r\n")) < 0){
        return;
    }

    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        return;
    }

    strftime(timebuf, 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", timestruct);
    timebuf[31] = '\0';

    if ((cnt = sprintf(buf, "Date: %s", timebuf)) < 0) {
        return;
    }
    if (write_to_socket(sd, FIELD_SERVER, sizeof(FIELD_SERVER), 0) < 0) {
        return;
    }
    if (write_to_socket(sd, "\r\n", 2, 0) < 0) {
        return;
    }
}

/* ======================== PRIVATE HELPER FUNCTIONS ======================== */

/* --------------------------------------------------------------------------
 *  send_date(sd, date, name, buf)
 * -------------------------------------------------------------------------- */
/*! \brief Formats and sends the given timestamp under the given name.
 *
 *  (The parameter "buf" is only included because it is more convenient to reuse
 *  the buffer in the public "send_response" function instead of allocating a
 *  new buffer in this function).
 *
 *  \param sd    The socket descriptor of the client.  The formatted date is
 *               sent through this descriptor.
 *  \param date  The timestamp to format and send.
 *  \param name  The name of the HTTP response fields which shall be used, e.g.
 *               "Date".
 *  \param buf   A buffer which can be used to write the response line, should
 *               have length MAX_SIZE_LINE
 *
 *  \return Either 0 when the function executed successfully, or -1 otherwise.
 */
static int
send_date(int sd, const time_t *date, char *name, char *buf) {

    int cnt;
    char timebuf[32];
    struct tm *timestruct = gmtime(date);
    strftime(timebuf, 32, "%a, %d %b %Y %H:%M:%S GMT\r\n", timestruct);
    timebuf[31] = '\0';

    if ((cnt = sprintf(buf, "%s: %s", name, timebuf)) < 0) {
        return -1;
    }
    if (write_to_socket(sd, buf, cnt, 0) < 0) {
        return -1;
    }

    return 0;
}


/* --------------------------------------------------------------------------
 *  send_cgi_output(sd_client, filename)
 * -------------------------------------------------------------------------- */
/*! \brief Executes the given CGI script and writes its output to sd_client.
 *
 *  This function will write error messages to stderr if any of the involved
 *  system calls fail.
 *
 *  \param sd_client  The socket descriptor to which the CGI script's output is
 *                    written.
 *  \param filename   The path of the CGI script (including tinyweb's root
 *                    directory).
 *
 *  \return On success, the number of bytes sent is returned, on error, -1 is
 *          returned.
 */
static int
send_cgi_output(int sd_client, const char *filename) {

    int pid, fd_pipe[2];
    int cnt = 0, bytes_sent = 0;

    /* initializes the two file handles for inter-process communication */
    if (pipe(fd_pipe) == -1) {
        perror("ERROR: pipe()");
        return -1;
    }

    if ((pid = fork()) < 0) {
        perror("ERROR: fork() before execle");
        return -1;
    }
    else if (pid > 0) { /* parent process */

        /* only the receiving end of the pipe is used in the parent process */
        close(fd_pipe[1]);
        char buf[MAX_SIZE_BUFFER_CGI];

        /* redirect everything and count how many bytes the child writes */
        do {
            cnt = read(fd_pipe[0], buf, MAX_SIZE_BUFFER_CGI);

            if (cnt < 0) {
                if (errno != EINTR) {
                    perror("ERROR: read() from pipe");
                    return -1;
                }
            }
            else {
                if (write(sd_client, buf, cnt) == -1) {
                    perror("ERROR: write() to socket");
                    return -1;
                }
                bytes_sent += cnt;
            }
        } while (cnt > 0);

        close(fd_pipe[0]);
        return bytes_sent;
    }
    else {
        /* In the child process the receiving end of the pipe is not required,
         * and therefore closed.  Both stdout and stderr of this process are
         * then redirected to the writing end of the pipe  */
        close(fd_pipe[0]);
        dup2(fd_pipe[1], STDOUT_FILENO);
        dup2(fd_pipe[1], STDERR_FILENO);
        execle(filename, filename, (char *)NULL, environ);

        /* if execle returns, something went wrong */
        perror("ERROR: execle() on CGI script");
        exit(EXIT_FAILURE);
    }
}
