/*! \file       content.c
 *  \author     Ralf Reutemann
 *  \brief      Provides funcitons to fetch http content information.
 *
 *  This file defines functions to fetch http content information in various
 *  formats.
 */
/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <string.h>
#include "content.h"

/*! \brief The content tyes and content information. */
static http_content_type_entry_t http_content_type_list[] = {
    { ".html",    "text/html"          },
    { ".css",     "text/css"           },
    { ".gif",     "image/gif"          },
    { ".jpg",     "image/jpeg"         },
    { ".pdf",     "application/pdf"    },
    { ".tar",     "application/x-tar"  },
    { ".xml",     "application/xml"    },
    { NULL,       "text/plain"         }
};

/* --------------------------------------------------------------------------
 *  get_http_content_type(const char *filename)
 * -------------------------------------------------------------------------- */
/*! \brief Processes filenames to fetch content types.
 *
 *  Fetches the content type based on the filname ending.
 *  
 *  \param filename     The filename used for content type detection.
 *
 *  \return             the http_content_type_t.
 */
http_content_type_t
get_http_content_type(const char *filename)
{
    int i;

    i = 0;
    while (http_content_type_list[i].ext != NULL) {
        if (strstr(filename, http_content_type_list[i].ext)) {
            break;
        } /* end if */
        i++;
    } /* end while */

    return (http_content_type_t)i;
} /* end of get_http_content_type */

/* --------------------------------------------------------------------------
 *  get_http_content_type_str(const http_content_type_t type)
 * -------------------------------------------------------------------------- */
/*! \brief Returns content type information as a string. 
 *
 *  Returns content type information as a string. 
 *
 *  \param type     The http_content_type to show string information about.
 *
 *  \return         The string definition of the given http_content_type.
 */
char *
get_http_content_type_str(const http_content_type_t type)
{
    return http_content_type_list[type].name;
} /* end of get_http_content_type_str */

