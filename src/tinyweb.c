/*===================================================================
 *
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <netdb.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>

#include "tinyweb.h"
#include "connect_tcp.h"
#include "passive_tcp.h"
#include "socket_io.h"

#include "safe_print.h"
#include "sem_print.h"
#include "http.h"
#include "request.h"
#include "response.h"
#include "log.h"


// Must be true for the server accepting clients,
// otherwise, the server will terminate
static volatile sig_atomic_t server_running = false;

#define IS_ROOT_DIR(mode)   (S_ISDIR(mode) && ((S_IROTH || S_IXOTH) & (mode)))

//
// TODO: Include your function header here
//
static void
sig_handler(int sig)
{
    int status;
    pid_t pid;

    switch(sig) {
        case SIGINT:
            // use our own thread-safe implemention of printf
            safe_printf("\n[%d] Server terminated due to keyboard interrupt\n", getpid());
            server_running = false;
            break;
        case SIGCHLD:
            while ((pid=wait3(&status, WNOHANG, (struct rusage *)0)) > 0) {
                safe_printf("\n[%d] Child finished, pid %d.\n", getpid(), pid);
            } /* end while */
            break;
        default:
            break;
    } /* end switch */
} /* end of sig_handler */


//
// TODO: Include your function header here
//
static void
print_usage(const char *progname)
{
  fprintf(stderr, "Usage: %s options\n", progname);
  // TODO: Print the program options
} /* end of print_usage */


//
// TODO: Include your function header here
//
static int
get_options(int argc, char *argv[], prog_options_t *opt)
{
    int                 c;
    int                 err;
    int                 success = 1;
    char               *p;
    struct addrinfo     hints;

    p = strrchr(argv[0], '/');
    if(p) {
        p++;
    } else {
        p = argv[0];
    } /* end if */

    opt->progname = (char *)malloc(strlen(p) + 1);
    if (opt->progname != NULL) {
        strcpy(opt->progname, p);
    } else {
        err_print("cannot allocate memory");
        return EXIT_FAILURE;
    } /* end if */

    opt->log_filename = NULL;
    opt->root_dir     = NULL;
    opt->server_addr  = NULL;
    opt->verbose      =    0;
    opt->timeout      =  120;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    while (success) {
        int option_index = 0;
        static struct option long_options[] = {
            { "file",    required_argument, 0, 0 },
            { "port",    required_argument, 0, 0 },
            { "dir",     required_argument, 0, 0 },
            { "verbose", no_argument,       0, 0 },
            { "debug",   no_argument,       0, 0 },
            { NULL,      0, 0, 0 }
        };

        c = getopt_long(argc, argv, "f:p:d:v", long_options, &option_index);
        if (c == -1) break;

        switch(c) {
            case 'f':
                // 'optarg' contains file name
                opt->log_filename = (char *)malloc(strlen(optarg) + 1);
                if (opt->log_filename != NULL) {
                    strcpy(opt->log_filename, optarg);
                } else {
                    err_print("cannot allocate memory");
                    return EXIT_FAILURE;
                } /* end if */
                break;
            case 'p':
                // 'optarg' contains port number
                if((err = getaddrinfo(NULL, optarg, &hints, &opt->server_addr)) != 0) {
                    fprintf(stderr, "Cannot resolve service '%s': %s\n", optarg, gai_strerror(err));
                    return EXIT_FAILURE;
                } /* end if */

                struct servent *pse = getservbyname(optarg, "http");
                if (pse != 0) {
                    opt->server_port = ntohs((unsigned short)pse->s_port);
                    printf("%d\n", opt->server_port);
                }
                else {
                    opt->server_port = (unsigned short)atoi(optarg);
                    printf("%d\n", opt->server_port);
                }
                break;
            case 'd':
                // 'optarg contains root directory */
                opt->root_dir = (char *)malloc(strlen(optarg) + 1);
                if (opt->root_dir != NULL) {
                    strcpy(opt->root_dir, optarg);
                } else {
                    err_print("cannot allocate memory");
                    return EXIT_FAILURE;
                } /* end if */
                break;
            case 'v':
                opt->verbose = 1;
                break;
            default:
                success = 0;
        } /* end switch */
    } /* end while */

    // check presence of required program parameters
    success = success && opt->server_addr && opt->root_dir;

    // additional parameters are silently ignored, otherwise check for
    // ((optind < argc) && success)

    return success;
} /* end of get_options */


static void
open_logfile(prog_options_t *opt)
{
    // open logfile or redirect to stdout
    if (opt->log_filename != NULL && strcmp(opt->log_filename, "-") != 0) {
        opt->log_fd = fopen(opt->log_filename, "w");
        if (opt->log_fd == NULL) {
            perror("ERROR: Cannot open logfile");
            exit(EXIT_FAILURE);
        } /* end if */
    } else {
        printf("Note: logging is redirected to stdout.\n");
        opt->log_fd = stdout;
    } /* end if */
} /* end of open_logfile */


static void
check_root_dir(prog_options_t *opt)
{
    struct stat stat_buf;

    // check whether root directory is accessible
    if (stat(opt->root_dir, &stat_buf) < 0) {
        /* root dir cannot be found */
        perror("ERROR: Cannot access root dir");
        exit(EXIT_FAILURE);
    } else if (!IS_ROOT_DIR(stat_buf.st_mode)) {
        err_print("Root dir is not readable or not a directory");
        exit(EXIT_FAILURE);
    } /* end if */
} /* end of check_root_dir */


static void
install_signal_handlers(void)
{
    struct sigaction sa;

    // init signal handler(s)
    // TODO: add other signals
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    } /* end if */

    struct sigaction sa2;
    sa2.sa_flags = 0;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_handler = sig_handler;
    if (sigaction(SIGCHLD, &sa2, NULL) < 0) {
        perror("sigaction(SIGCHLD)");
        exit(EXIT_FAILURE);
    }
} /* end of install_signal_handlers */


int
main(int argc, char *argv[])
{
    int retcode = EXIT_SUCCESS;
    prog_options_t my_opt;

    // read program options
    if (get_options(argc, argv, &my_opt) == 0) {
        print_usage(my_opt.progname);
        exit(EXIT_FAILURE);
    } /* end if */

    // set the time zone (TZ) to GMT in order to
    // ignore any other local time zone that would
    // interfere with correct time string parsing
    setenv("TZ", "GMT", 1);
    tzset();

    // do some checks and initialisations...
    open_logfile(&my_opt);
    check_root_dir(&my_opt);
    install_signal_handlers();
    init_logging_semaphore();

    set_logfile(my_opt.log_fd);

    // TODO: start the server and handle clients...
    // here, as an example, show how to interact with the
    // condition set by the signal handler above
    printf("[%d] Starting server '%s'...\n", getpid(), my_opt.progname);
    server_running = true;

    /* passive_tcp prints error messages internally */
    int sd_server = passive_tcp(my_opt.server_port, 5);
    if (sd_server == -1) {
        exit(EXIT_FAILURE);
    }

    while(server_running) {

        int pid;

        socklen_t client_sa_len;
        struct sockaddr_in client_sa;
        client_sa_len = sizeof(client_sa);


        int sd_client = accept(sd_server,
                (struct sockaddr *)&client_sa,
                &client_sa_len);

        if (sd_client == -1) {
            if (errno != EINTR) {
                perror("ERROR: accept()");
                exit(EXIT_FAILURE);
            }
        }
        else {

            if ((pid = fork()) < 0) {
                perror("ERROR: fork()");
                send_static_500(sd_client);
                shutdown(sd_client, SHUT_WR);
                exit(EXIT_FAILURE);
            }
            else if (pid > 0) {  /* parent process */
                close(sd_client);
            }
            else {               /* child process */
                close(sd_server);

                int cnt;
                struct sockaddr_in addr;
                socklen_t addr_size = sizeof(struct sockaddr_in);
                if ((cnt = getpeername(sd_client, (struct sockaddr *)&addr,
                                &addr_size)) < 0) {
                    perror("ERROR: getpeername()");
                    send_static_500(sd_client);
                    shutdown(sd_client, SHUT_WR);
                    exit(EXIT_FAILURE);
                }
                char client_ip[20];
                client_ip[cnt-1] = '\0';
                strcpy(&client_ip[0], inet_ntoa(addr.sin_addr));


                char buf[MAX_SIZE_REQUEST];
                if (read_from_socket(sd_client, &buf[0], MAX_SIZE_REQUEST, 0)
                        < 0) {
                    perror("ERROR: read_from_socket()");
                    send_static_500(sd_client);
                    shutdown(sd_client, SHUT_WR);
                    exit(EXIT_FAILURE);
                }

                request_t req;

                int status = parse_request(&buf[0], &req);

                char filename[MAX_SIZE_URI];
                if (sprintf(filename, "%s%s", my_opt.root_dir, req.uri) < 0) {
                    fprintf(stderr, "ERROR: sprintf()");
                    send_static_500(sd_client);
                    shutdown(sd_client, SHUT_WR);
                    exit(EXIT_FAILURE);
                }

                // TODO remove before shipping
                print_log(stderr, "FILENAME: %s\n", filename);
                print_log(stderr, "METHOD:   %d\n", req.method);
                print_log(stderr, "STATUS:   %d\n", http_status_list[status].code);
                print_log(stderr, "URI:      %s\n", req.uri);
                print_log(stderr, "RANGE:    %d\n", req.range_start);

                // TODO handle our own error codes
                response_t res;
                generate_response_header(filename, status, &req, &res);

                if ((cnt = send_response(sd_client, filename, &res)) < 0) {
                    fprintf(stderr, "ERROR: send_response()");

                    // this might not work, but we can try.  send_static_500
                    // will handle all further errors.
                    send_static_500(sd_client);
                    shutdown(sd_client, SHUT_WR);
                    exit(EXIT_FAILURE);
                }

                // in parse_request, we put a '\0' at the end of the first line,
                // so the use of &buf[0] below is "safe"
                log_request(client_ip, res.date, &buf[0], res.status, cnt);
                shutdown(sd_client, SHUT_WR);
                exit(EXIT_SUCCESS);
            }
        }
    } /* end while */

    fclose(my_opt.log_fd);
    printf("[%d] Good Bye...\n", getpid());
    exit(retcode);
} /* end of main */

