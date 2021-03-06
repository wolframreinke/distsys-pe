/*! \file       log.c
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Functions to define logfile and write log-messages into logfile
 *
 *  See log.h for API documentation.
 */
#include <stdio.h>
#include <time.h>

#include "sem_print.h"

#include "log.h"
#include "http.h"

/* we chose to use a global variable because it seemed more difficult to pass
 * around the FILE pointer everywhere we write log messages */

/*! The file to which log entries are written */
static FILE *logfile = 0;

/* --------------------------------------------------------------------------
 *  set_logfile(lf)
 * -------------------------------------------------------------------------- */
/*!
 * \brief Sets the file to which log entries are written
 *
 * The logfile-pointer is set to the lf-pointer given to the method as parameter
 *
 * \param lf  The log file
 */
void
set_logfile(FILE *lf) {
    logfile = lf;
}

/* --------------------------------------------------------------------------
 *  log_request(host, date, request_first_line, status, bytes_sent)
 * -------------------------------------------------------------------------- */
/*!
 * \brief Writes a log entry into the log file.
 *
 * The written entry has the format
 *      <host> - - [<date>] "<request>" <status> <bytes-sent>
 *  where
 *      <host>       The IP address from which the request was sent
 *      <date>       The date of the request.
 *      <request>    The first line of the HTTP request
 *      <status>     The status code of the HTTP response sent by the server
 *      <bytes-sent> The number of bytes sent to the client
 *
 * \param host                A string containing the ip address of the host
 * \param date                The date and time when the response was sent
 * \param request_first_line  The first line of the HTTP request
 * \param status              The status of the sent response
 * \param bytes_sent          The number of bytes sent to the client
 */
void
log_request(const char *host, time_t date, const char *request_first_line,
        http_status_t status, size_t bytes_sent) {

    char timebuf[32];
    struct tm *timestruct = gmtime(&date);
    strftime(timebuf, 32, "%d/%b/%Y:%H:%M:%S %z", timestruct);
    timebuf[31] = '\0';

    fprintf(logfile, "%s - - [%s] \"%s\" %d %zd\n",
            host,
            timebuf,
            request_first_line,
            http_status_list[status].code,
            bytes_sent);
}
