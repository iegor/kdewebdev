
/***************************************************************************
                          xsldbg.h  - describe the application level functions
                             -------------------
    begin                : Sun Sep 16 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef XSLDEBUGGER_H
#define XSLDEBUGGER_H

#ifdef USE_KDE_DOCS

/**
 * Provide provide application level services and useful bits and pieces
 *
 * @short application functions and useful bits and pieces
 *
 * @author Keith Isdale <k_isdale@tpg.com.au>
 */
#endif

 
/* We want skip most of these includes when building documentation */
#ifndef BUILD_DOCS
#ifndef __riscos
#  ifndef WIN32
#     include "config.h"
      /* we don't need to use dll declares for *nix based machines */
#     define XSLDBG_SO_API
#  else
      /* dll declares get defined in xsldbgwin32config.h *nix based machines */
#    include "xsldbgwin32config.h"
#  endif
#else
   /* we don't need to use dll declares for risc_os*/
#  define XSLDBG_SO_API
#  include "config_riscos.h"
#  include "libxml/riscos.h"
#endif

#include <libxslt/xsltconfig.h>


#if !defined (FORCE_DEBUGGER) &&  (LIBXSLT_VERSION > 10019)
#  ifndef WITH_XSLT_DEBUGGER
#     ifndef WITH_DEBUGGER
#       error "WITH_DEBUGGER MACRO not defined in libxslt maybe you've disable debugger in libxslt." \
                "if your are sure then disable this check by defining WITH_FORCE_DEBUGGER. eg.\n" \
                 "\nmake CFLAGS=\"$CFLAGS -D FORCE_DEBUGGER\""
#     endif
#  endif
#endif

#ifdef  WITH_XSLDBG_DEBUG

#ifndef WITH_XSLDBG_DEBUG_PROCESS
#define WITH_XSLDBG_DEBUG_PROCESS
#endif

#ifndef WITH_XSLDBG_DEBUG_BREAKPOINTS
#define WITH_XSLDBG_DEBUG_BREAKPOINTS
#endif

#endif /* end of WITH_XSL_DEBUG */


#include <libxslt/xslt.h>
#include <libexslt/exslt.h>
#include <libxslt/xsltutils.h>

#include <qstring.h>
#include <klocale.h>

#include "breakpoint.h"


#endif /* BUILD_DOCS */


#ifdef __cplusplus
extern "C" {
#endif


#ifdef USE_GNOME_DOCS

/**
 * xsldbgLoadStylesheet:
 *
 * Load the stylesheet and return it
 *
 * Returns the stylesheet after reloading it if successful
 *         NULL otherwise
 */
#else
#ifdef USE_KDE_DOCS

/**
 * Load the stylesheet and return it
 *
 * @returns The stylesheet after reloading it if successful
 *         NULL otherwise
 */
#endif
#endif
    xsltStylesheetPtr xsldbgLoadStylesheet(void);


#ifdef USE_GNOME_DOCS

/**
 * xsldbgLoadXmlData:
 *
 * Load the xml data file and return it
 *
 * Returns the data file after reloading it if successful
 *         NULL otherwise
 */
#else
#ifdef USE_KDE_DOCS

/**
 * Load the xml data file and return it
 *
 * @returns The stylesheet after reloading it if successful
 *         NULL otherwise
 */
#endif
#endif
    xmlDocPtr xsldbgLoadXmlData(void);


#ifdef USE_GNOME_DOCS

/**
 * xsldbgLoadXmlTemporary:
 * @path: The name of temporary file to load
 *
 * Load the temporary data file and return it
 *
 * Returns The temporary file after reloading it if successful,
 *         NULL otherwise
 */
#else
#ifdef USE_KDE_DOCS

/**
 * Load the temporary data file and return it
 *
 * @returns The temporary file after reloading it if successful,
 *         NULL otherwise
 * @param path The name of temporary file to loa
 */
#endif
#endif
    xmlDocPtr xsldbgLoadXmlTemporary(const xmlChar * path);


void xsldbgGenericErrorFunc(void *ctx, const char *msg, ...)
#ifdef __GNUC__
      __attribute__ ( ( format ( printf, 2, 3 ) ) )
#endif
;
int xsldbgMain(int argc, char **argv);

#ifdef __cplusplus
}
#endif

void xsldbgGenericErrorFunc(QString const &text);
QString xsldbgUrl(const char *utf8Url); 
QString xsldbgUrl(const xmlChar *utf8Url);
QString xsldbgText(const char *utf8Text);
QString xsldbgText(const xmlChar *utf8Text);

#endif
