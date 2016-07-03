/*! \file       sem_print.h
 *  \author     Ralf Reutemann
 *  \brief      sem_print method declaration.
 *
 *  This file defines sem_print method declarations.
 */
 /*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#ifndef _SEM_PRINT_H
#define _SEM_PRINT_H

#include <semaphore.h>


extern void init_logging_semaphore(void);
extern void set_verbosity_level(unsigned short level);
extern int print_log(FILE *file, const char *format, ...);
extern int print_debug(const char *format, ...);
extern void print_http_header(const char *what, const char *response_str);

#endif

