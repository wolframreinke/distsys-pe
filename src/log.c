#include <stdio.h>
#include <time.h>

#include "sem_print.h"

#include "log.h"
#include "http.h"

static FILE *logfile = 0;

void
set_logfile(FILE *lf) {
    logfile = lf;
}

void
log_request(const char *host, time_t date, const char *request_first_line,
        http_status_t status, size_t bytes_sent) {


    char timebuf[32];
    struct tm *timestruct = localtime(&date);
    // TODO might be wrong
    strftime(timebuf, 32, "%d/%b/%Y:%H:%M:%S %z", &timestruct[0]);
    timebuf[31] = '\0';

    fprintf(logfile, "%s - - [%s] \"%s\" %d %zd\n",
            host,
            timebuf,
            request_first_line,
            http_status_list[status].code,
            bytes_sent);
}
