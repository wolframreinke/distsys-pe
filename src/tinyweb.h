/*! \file       tinyweb.h
 *  \author     Ralf Reutemann
 *  \author     Fabian Haag
 *  \author     Wolfram Reinke
 *  \author     Martin Schmid
 *  \date       July 22, 2016
 *  \brief      Tinyweb program options and general configuration.
 *
 *  This file defines program options and for the Tinyweb web server.
 */

#ifndef _TINYWEB_H
#define _TINYWEB_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdbool.h>

#define err_print(s)              fprintf(stderr, "ERROR: %s, %s:%d\n", (s), __FILE__, __LINE__)

#define BUFFER_SIZE                      8192
#define DEFAULT_HTML_PAGE      "default.html"


/*! \brief The program options accepted by Tinyweb. */
typedef struct prog_options {
    char            *progname;     /*!< The name of the program             */
    char            *root_dir;     /*!< The root directory for web contents */
    char            *log_filename; /*!< The filename of the log file        */
    FILE            *log_fd;       /*!< The file descriptor of the log file */
    bool             verbose;      /*!< If more output should be printed    */
    unsigned short   timeout;      /*!< (not used)                          */
    struct addrinfo *server_addr;  /*!< The address info for the server     */
    int              server_port;  /*!< The port, this server serves        */
} prog_options_t;

#endif

