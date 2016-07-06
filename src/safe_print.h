/*! \file       safe_print.h
 *  \author     Ralf Reutemann
 *  \date       July 22, 2016
 *  \brief      Functions to coordinate the console messages sent by the sig-
 *				handlers.
 *
 *  This module provides a function to safely print the asynchronous console-
 *  messages coming from the sihnal-handlers.
 */

#ifndef _SAFE_PRINT_H
#define _SAFE_PRINT_H

extern int safe_printf(const char *format, ...);

#endif

