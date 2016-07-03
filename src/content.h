/*! \file       content.h
 *  \author     Ralf Reutemann
 *  \brief      HTTP content information.
 *
 *  This file defines commonly used HTTP content types.
 */
/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/
 
#ifndef _CONTENT_H
#define _CONTENT_H

/*! \brief The HTTP content types accepted by Tinyweb. */
typedef enum http_content_type {
    HTTP_CONTENT_TYPE_HTML = 0,
    HTTP_CONTENT_TYPE_CSS,
    HTTP_CONTENT_TYPE_GIF,
    HTTP_CONTENT_TYPE_JPEG,
    HTTP_CONTENT_TYPE_PDF,
    HTTP_CONTENT_TYPE_TAR,
    HTTP_CONTENT_TYPE_XML,
    HTTP_CONTENT_TYPE_DEFAULT
} http_content_type_t;

/*! \brief The http method entry consisting of name and text. */
typedef struct http_content_type_entry {
    char                *ext;
    char                *name;
} http_content_type_entry_t;


extern http_content_type_t
get_http_content_type(const char *filename);

extern char *
get_http_content_type_str(const http_content_type_t type);

#endif

