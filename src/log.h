#ifndef _LOG_H_
#define _LOG_H_

#include "http.h"

void
set_logfile(FILE *logfile);

void
log_request(const char *host, time_t date, const char *request_first_line,
        http_status_t status, size_t bytes_sent);

#endif // _LOG_H_
