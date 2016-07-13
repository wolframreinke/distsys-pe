/*! \file       safe_print.c
 *  \author     Ralf Reutemann
 *  \date       July 22, 2016
 *  \brief      Functions to coordinate the console messages sent by the sig-
 *              handlers
 *
 *  See safe_print.h for API documentation.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
/* --------------------------------------------------------------------------
 *  safe_printf(format, ...)
 * -------------------------------------------------------------------------- */
 /*! \brief Projects the async-signal to a format with given length
 *
 *  This method is an async-signal-safe wrapper for the printf-method. It allows
 *  to print the asynchronous messages from the signal-handlers to be printed
 *  on the stdout without interfering each other.
 *
 *  \param  format  The Format, the async-signal should be projected to
 *
 *  \return Number of Bytes written (as Integer)
 */
#define MAXS 1024
int
safe_printf(const char *format, ...)
{
    char buf[MAXS];
    ssize_t cc;
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    cc = write(STDOUT_FILENO, buf, strlen(buf));/* write is async-signal-safe */

    return (int)cc;
} /* end of safe_printf */

