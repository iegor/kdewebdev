
/***************************************************************************
                          debugXSL.c  - debugger commands to use
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

/*
 * Orinal file : debugXML.c : This is a set of routines used for 
 *              debugging the tree produced by the XML parser.
 *
 * New file : shell.c : Debug support version
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 * 
 * Permission obtained to modify the LGPL'd code and extend to include breakpoints, inspections of
 * stylesheet source, xml data, stylesheet variables
 */

#ifdef WIN32
#include <wtypes.h>
#include <winbase.h>            /* needed fort the sleep function */
#endif

#include "xsldbg.h"
#include "files.h"
#include "cmds.h"               /* list of command Id's */
#include "debug.h"
#include "debugXSL.h"
#include "options.h"
#include "breakpoint.h"
#include "help.h"
#include <stdlib.h>
#include <libxslt/transform.h>  /* needed by source command */
#include <libxslt/xsltInternals.h>
#include <libxml/debugXML.h>
#include <stdio.h>

/* language change support */
#ifdef LOCALE_PREFIX
#include <locale.h>
#endif

#include "xsldbgmsg.h"
#include "xsldbgthread.h"       /* for get thread status */
#include "xsldbgio.h"

/* current template being processed */
xsltTemplatePtr rootCopy;

/* how may items have been printed */
int printCount;

/* used to sending small amounts data when xsldbg runs as a thread */
xmlChar messageBuffer[2000];

/* To achieve the same fucntionality of a next command
   we first do a step, then a step up */
int nextCommandActive = 0;

/* Do we print the values for watches each time the debugger stops */
int showWatchesActive = 1;

extern FILE *terminalIO;

int xsldbgStop = 0;
int xsldbgValidateBreakpoints = BREAKPOINTS_NEED_VALIDATION;
int xsldbgHasLineNumberFix;  
bool xsldbgReachedFirstTemplate = false;

/* valid commands of xslDbgShell */
const char *commandNames[] = {
    "help",

    "bye",
    "exit",
    "quit",

    "step",
    "stepup",
    "stepdown",
    "next",                      /* next ie step over template function call*/
    "continue",
    "run",

    "templates",
    "where",
    "frame",
    "stylesheets",

    "break",
    "showbreak",
    "delete",
    "enable",
    "disable",

    "ls",
    "dir",
    "du",
    "cat",
    "print",
    "pwd",
    "dump",
    "base",

    "globals",
    "locals",
    /* "cat", already listed */
    "source",
    "data",
    "output",                   /* output file name */
    "cd",

    /* file related */
    /* "output", already listed */
    "entities",
    "system",
    "public",
    "encoding",
    "validate",
    "load",
    "save",
    "write",
    "free",

    /* Operating system related */
    "chdir",
    "shell",
    "tty",

    /* libxslt parameter related */
    "addparam",
    "delparam",
    "showparam",
    "setoption",
    "options",

    /* extra options */
    "trace",
    "walk",
    "addwatch",
    "delwatch",
    "showwatch",

    /* searching */
    "search",

    /*variable value change */
    "set",
    
    /* language change */
    "lang",

    NULL                        /* Indicate the end of list */
};

/* valid commands of xslShell in there alternative|shorter format */
const char *shortCommandNames[] = {
    "h",

    "bye",
    "exit",
    "q",                        /*quit */

    "s",                        /* step */
    "up",                       /*stepup */
    "down",                     /* stepdown */
    "n",                        /* next ie step over function call*/
    "c",                        /* continue */
    "r",                        /* run */

    "t",                        /* templates */
    "w",                        /* where */
    "f",
    "style",

    "b",                        /* break */
    "show",
    "d",                        /* delete */
    "e",                        /* enabled */
    "disable",

    "ls",
    "dir",
    "du",
    "cat",
    "print",
    "pwd",
    "dump",
    "base",

    "globals",
    "locals",
    /* "cat", already listed */
    "source",
    "data",
    "o",                        /* output file name */
    "cd",


    /* file related */
    /* "output", already listed */
    "ent",                      /* entities command */
    "sys",                      /* sytem command */
    "pub",                      /* public command */
    "encoding",
    "validate",
    "load",
    "save",
    "write",
    "free",

    /* Operating system related */
    "chdir",
    "shell",
    "tty",

    /* libxslt parameter related */
    "addparam",
    "delparam",
    "showparam",
    "setoption",
    "options",

    /* extra options/commands */
    "trace",
    "walk",
    "watch",
    "delwatch",
    "watches",

    /* searching */
    "search",

    /*variable value change */
    "set",

    /* language change */
    "lang",

    NULL                        /* Indicate the end of list */
};


/* some convenient short cuts when using  cd command*/
const char *cdShortCuts[] = {
    "<<",
    ">>",
    "<-",
    "->",
    NULL                        /* indicate end of list */
};

/* what to replace shortcuts with */
const char *cdAlternative[] = {
    "preceding-sibling::node()",
    "following-sibling::node()",
    "ancestor::node()",
    "descendant::node()",
    NULL                        /* indicate end of list */
};

/* what enum to use for shortcuts */
enum ShortcutsEnum {
    DEBUG_PREV_SIBLING = 200,
    DEBUG_NEXT_SIBLING,
    DEBUG_ANCESTOR_NODE,
    DEBUG_DESCENDANT_NODE
};



#include <libxml/xpathInternals.h>

#include <libxslt/extra.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif


#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>


/* -----------------------------------------
   Private function declarations for debugXSL.c
 -------------------------------------------*/


/* xslDbgCd :
 * Change directories
 * @styleCtxt : current stylesheet context
 * @ctxt : current shell context
 * @arg : path to change to
 * @source : is valid
 *
 * Returns 1 on success,
 *         0 otherwise
 */
int xslDbgCd(xsltTransformContextPtr styleCtxt, xmlShellCtxtPtr ctxt,
             xmlChar * arg, xmlNodePtr source);


/**
 * xslDbgPrintCallStack:
 * @arg : the number of frame to print, NULL if all items 
 * 
 * Print all items found on the callStack
 *
 * Returns 1 on success,
 *         0 otherwise
 */
int xslDbgPrintCallStack(const xmlChar * arg);


/**
 * xslDbgSleep:
 * @delay : the number of microseconds to delay exection by
 *
 * Delay execution by a specified number of microseconds. On some system 
 *      this will not be at all accurate.
 */
void xslDbgSleep(long delay);


/**
 * xslDbgWalkContinue:
 *
 * Delay execution for time as indicated by OPTION_WALK_SPEED
 * Can only be called from within shellPrompt!
 * OPTION_WALK_SPEED != WALKSPEED_STOP
 *
 * Returns 1 if walk is to continue,
 *         0 otherwise
 */
int xslDbgWalkContinue(void);



/**
 * addBreakPointNode:
 * @payload : valid breakPointPtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addBreakPointNode(void *payload, void *data,
                    xmlChar * name);


/**
 * addSourceNode:
 * @payload : valid xsltStylesheetPtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addSourceNode(void *payload, void *data,
                xmlChar * name);


/**
 * addTemplateNode:
 * @payload : valid xsltTemplatePtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addTemplateNode(void *payload, void *data,
                  xmlChar * name);

/**
 * addGlobalNode:
 * @payload : valid xmlNodePtr of global variable
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addGlobalNode(void *payload, void *data,
                xmlChar * name);

/**
 * addLocalNode:
 * @payload : valid xmlNodePtr of local variable
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addLocalNode(void *payload, void *data,
               xmlChar * name);


/**
 * addIncludeNode:
 * @payload : valid xmlNodePtr of include instuction
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
  addIncludeNode(void *payload, void *data,
                 xmlChar * name);



/**
 * addCallStackItems:
 * 
 * Convert call stack items into format needed, and add to search dataBase 
 */
void
  addCallStackItems(void);


/**
 *  shellPrompt:
 * @source: The current stylesheet instruction being executed
 * @doc: The current document node being processed
 * @filename: Not used
 * @input: The function to call to when reading commands from stdio
 * @output: Where to put the results
 * @styleCtxt: Is valid 
 *
 * Present to the user the xsldbg shell
 */
void shellPrompt(xmlNodePtr source, xmlNodePtr doc,
                 xmlChar * filename,
                 xmlShellReadlineFunc input,
                 FILE * output, xsltTransformContextPtr styleCtxt);

/* ------------------------------------- 
    End private functions
---------------------------------------*/



/**
 * debugXSLGetTemplate:
 * 
 * Return the last template node found, if an
 *
 * Returns The last template node found, if any
 */
xsltTemplatePtr
debugXSLGetTemplate(void)
{
    return rootCopy;
}


/****************************************************************
 *								*
 *	 	The XSL shell related functions			*
 *								*
 ****************************************************************/


/* xslDbgCd :
 * Change directories
 * @styleCtxt : current stylesheet context
 * @ctxt : current shell context
 * @arg : path to change to and in UTF-8
 * @source : is valid 
 *
 * Returns 1 on success,
 *         0 otherwise
 */
int
xslDbgCd(xsltTransformContextPtr styleCtxt, xmlShellCtxtPtr ctxt,
         xmlChar * arg, xmlNodePtr source)
{
    xmlXPathObjectPtr list = NULL;
    int result = 0;
    int offset = 2;             /* in some cases I'm only interested after first two chars  */

    if (!ctxt) {
        xsldbgGenericErrorFunc(i18n("Error: Debugger has no files loaded. Try reloading files.\n"));
        return result;
    }

    if (arg == NULL)
        arg = (xmlChar *) "";
    if (arg[0] == 0) {
        ctxt->node = (xmlNodePtr) ctxt->doc;
    } else {
        if ((arg[0] == '-') && (xmlStrLen(arg) > 2)) {
            if (styleCtxt) {
                if (arg[1] == 't') {
                    xmlNodePtr templateNode;

                    /* quickly find a template */
                    /* skip any white spaces */
                    while (_IS_BLANK(arg[offset]))
                        offset++;

                    templateNode =
                        findTemplateNode(styleCtxt->style, &arg[offset]);
                    if (!templateNode) {
                        xsldbgGenericErrorFunc(i18n("Error: The XSLT template named \"%1\" was not found.\n").arg(xsldbgText(&arg[offset])));
                        return result;
                    } else {
                        xsldbgGenericErrorFunc(i18n(" template: \"%1\"\n").arg(xsldbgText(&arg[offset])));
                        ctxt->node = templateNode;
                        result = 1;
                        return result;
                    }
                } else if (arg[1] == 's') {
                    /*quickly switch to another stylesheet node */
                    xmlXPathContextPtr pctxt;

                    if (source) {
                        pctxt = xmlXPathNewContext(source->doc);
                        if (pctxt == NULL) {
                            xmlFree(ctxt);
                            /* xslDebugStatus = DEBUG_QUIT; */
                            return result;
                        }
                        if (!xmlXPathNsLookup(pctxt, (xmlChar *) "xsl"))
                            xmlXPathRegisterNs(pctxt, (xmlChar *) "xsl",
                                               XSLT_NAMESPACE);
                        list =
                            xmlXPathEval((xmlChar *) & arg[offset], pctxt);
                        if (pctxt) {
                            xmlFree(pctxt);
                        }
                    } else {
                        xsldbgGenericErrorFunc(i18n("Error: Unable to cd. No stylesheet loaded.\n"));
                    }
                } else {
                    xsldbgGenericErrorFunc(i18n("Error: Unknown arguments to the command %1.\n").arg("cd"));
                }
            } else
                xsldbgGenericErrorFunc(i18n("Error: Unable to cd. No stylesheet loaded.\n"));
        } else {
            xmlNodePtr savenode;

            if (styleCtxt) {
                savenode = styleCtxt->xpathCtxt->node;
                ctxt->pctxt->node = ctxt->node;
                styleCtxt->xpathCtxt->node = ctxt->node;
                if (!xmlXPathNsLookup(ctxt->pctxt, (xmlChar *) "xsl"))
                    xmlXPathRegisterNs(ctxt->pctxt, (xmlChar *) "xsl",
                                       XSLT_NAMESPACE);
                list = xmlXPathEval((xmlChar *) arg, styleCtxt->xpathCtxt);
                styleCtxt->xpathCtxt->node = savenode;
            } else if (ctxt->pctxt) {
                if (!xmlXPathNsLookup(ctxt->pctxt, (xmlChar *) "xsl"))
                    xmlXPathRegisterNs(ctxt->pctxt, (xmlChar *) "xsl",
                                       XSLT_NAMESPACE);
                list = xmlXPathEval((xmlChar *) arg, ctxt->pctxt);
            } else {
                xsldbgGenericErrorFunc(i18n("Error: Invalid arguments to the command %1.\n").arg("cd"));
            }
        }

        if (list != NULL) {
            switch (list->type) {
                case XPATH_NODESET:
                    if (list->nodesetval) {
                        if (list->nodesetval->nodeNr == 1) {
                            ctxt->node = list->nodesetval->nodeTab[0];
                            /* tell the application about the new line
                             * number we are looking at */
                            if (getThreadStatus() == XSLDBG_MSG_THREAD_RUN) {
                                int breakpoint = 0;

                                xsldbgUpdateFileDetails(ctxt->node);
                                notifyXsldbgApp(XSLDBG_MSG_LINE_CHANGED,
                                                &breakpoint);
                            }
                            result = 1;
                        } else
                            xsldbgGenericErrorFunc(i18n("Warning: XPath %1 is a Node Set with %n child.", "Warning: XPath %1 is a Node Set with %n children.", list->nodesetval->nodeNr).arg(xsldbgText(arg)) + QString("\n"));
                    } else {
                        xsldbgGenericErrorFunc(i18n("Warning: XPath %1 is an empty Node Set.\n").arg(xsldbgText(arg)));
                    }
                    break;

                default:
                    xmlShellPrintXPathError(list->type, (char *) arg);
            }
            xmlXPathFreeObject(list);
        } else {
            xsldbgGenericErrorFunc(i18n("Error: XPath %1 was not found.\n").arg(xsldbgText(arg)));
        }
        if (ctxt->pctxt)
          ctxt->pctxt->node = NULL;
    }
    return result;
}


/**
 * xslDbgPrintCallStack:
 * @arg : the number of frame to print, NULL if all items 
 * 
 * Print all items found on the callStack
 *
 * Returns 1 on success,
 *         0 otherwise
 */
int
xslDbgPrintCallStack(const xmlChar * arg)
{
    int depth = 0;
    int result = 1;
    callPointPtr callPointItem;

    if (arg == NULL) {
        if (getThreadStatus() == XSLDBG_MSG_THREAD_RUN) {
            notifyListStart(XSLDBG_MSG_CALLSTACK_CHANGED);
            /* we send the oldest frame stack first */
            for (depth = 1; depth <= callStackGetDepth(); depth++) {
                callPointItem = callStackGet(depth);
                if (callPointItem && callPointItem->info) {
                    notifyListQueue(callPointItem);
                }
            }
            notifyListSend();
        } else {
	    xmlChar *nameTemp, *modeTemp; 
            for (depth = callStackGetDepth(); depth >= 1; depth--) {
                callPointItem = callStackGet(depth);
		nameTemp = NULL;
		modeTemp = NULL;
                if (callPointItem && callPointItem->info) {
                    if (depth == callStackGetDepth()) {
                        xmlChar *curUrl = xsldbgUrl();
                        long curLine = xsldbgLineNo();
                        /* if possible list the current location */
                        if (rootCopy && (rootCopy->match || rootCopy->name)
                            && curUrl) {			    
			    xmlChar *rootNameTemp, *rootModeTemp;
			    rootNameTemp = fullQName(rootCopy->nameURI, rootCopy->name);
			    rootModeTemp = fullQName(rootCopy->modeURI, rootCopy->mode);
			    if (rootNameTemp && rootModeTemp){
			      if (rootCopy->match)
			        /* display information about the current XSLT template */
                                xsldbgGenericErrorFunc(i18n("#%1 template: \"%2\" mode: \"%3\"").arg(depth).arg(xsldbgText(rootCopy->match)).arg(xsldbgText(rootModeTemp)));
			      else
			        /* display information about the current XSLT template */
                                xsldbgGenericErrorFunc(i18n("#%1 template: \"%2\" mode: \"%3\"").arg(depth).arg(xsldbgText(rootNameTemp)).arg(xsldbgText(rootModeTemp)));
			      /* display where we are in the source/document file */	
			      xsldbgGenericErrorFunc(i18n(" in file \"%1\" at line %2\n").arg(xsldbgUrl(curUrl)).arg(curLine));
			    }else{
			      xsldbgGenericErrorFunc(i18n("Error: Out of memory.\n"));
			      result = 0;
			    }
			    if (rootNameTemp){
			      xmlFree(rootNameTemp);
			      rootNameTemp = NULL;
			    }
			    if (rootModeTemp){
			      xmlFree(rootModeTemp);
			      rootModeTemp = NULL;
			    }
                        } else if (curUrl) {
			    /* display information about the current XSLT template */
                            xsldbgGenericErrorFunc(i18n("#%1 template: \"LIBXSLT_DEFAULT\" mode: \"\"").arg(depth));
			    /* display where we are in the source/document file */	
                            xsldbgGenericErrorFunc(i18n(" in file \"%1\" at line %2\n").arg(xsldbgUrl(curUrl)).arg(curLine));
                        }
                        if (curUrl)
                            xmlFree(curUrl);

                    }
		    nameTemp = fullQName(callPointItem->info->templateURI,
					 callPointItem->info->templateName);
		    modeTemp = fullQName(callPointItem->info->modeURI,
					 callPointItem->info->modeName);
		    if (nameTemp && modeTemp){
		      /* display information about the current XSLT template */
		      xsldbgGenericErrorFunc(i18n("#%1 template: \"%2\" mode: \"%3\"").arg(depth - 1).arg(xsldbgText(nameTemp)).arg(xsldbgText(modeTemp)));
		      if (callPointItem->info->url)
			/* display where we are in the source/document file */	
                        xsldbgGenericErrorFunc(i18n(" in file \"%1\" at line %2\n").arg(xsldbgUrl(callPointItem->info->url)).arg(callPointItem->lineNo));
		      else
                        xsldbgGenericErrorFunc("\n");
		    }else{
		      xsldbgGenericErrorFunc(i18n("Error: Out of memory.\n"));
		      result = 0;
		    }
		    if (nameTemp){ 
		      xmlFree(nameTemp);
		      nameTemp = NULL;
		    }
		    if(modeTemp){
		      xmlFree(modeTemp);
		      modeTemp = NULL;
		    }
		    
                } else {
#ifdef WITH_XSLDBG_DEBUG_PROCESS
                    xsltGenericError(xsltGenericErrorContext,
                                     "Error: Call stack item not found at depth %d : xslDbgPrintCallStack\n", depth);
#endif
		    result = 0;
                    break;
                }
            }
            if (callStackGetDepth() == 0)
                xsldbgGenericErrorFunc(i18n("\tNo items in call stack.\n"));
            else
                xsldbgGenericErrorFunc("\n");
        }
    } else {
        long templateDepth = atol((char *) arg);


        if (getThreadStatus() == XSLDBG_MSG_THREAD_RUN) {
            /* should never happen but just in case, when running as a 
             * thread always provide NO params to the where command */
#ifdef WITH_XSLDBG_DEBUG_PROCESS
            xsltGenericError(xsltGenericErrorContext,
                             "Error: Notification of a frame not supported\n");
#endif
	    result = 0;
            return result;
        }

        if (templateDepth >= 0) {
            callPointItem = callStackGet(templateDepth + 1);
            if (callPointItem && callPointItem->info) {
		/* display information about the current XSLT template */
                xsldbgGenericErrorFunc(i18n("#%1 template: \"%2\"").arg(templateDepth).arg(xsldbgText(callPointItem->info->templateName)));
                /* should alays be present but .. */
                if (callPointItem->info->url)
		    /* display where we are in the source/document file */	
                    xsldbgGenericErrorFunc(i18n(" in file \"%1\" at line %2\n").arg(xsldbgUrl(callPointItem->info->url)).arg(callPointItem->lineNo));
                else
                    xsldbgGenericErrorFunc("\n");
            } else {
#ifdef WITH_XSLDBG_DEBUG_PROCESS
                xsltGenericError(xsltGenericErrorContext,
                                 "Error: Call stack item not found at templateDepth %d : xslDbgPrintCallStack\n", depth);
#endif
		result = 0;
            }
        }
    }
    return result;
}


/**
 * xslDbgSleep:
 * @delay : the number of microseconds to delay exection by
 *
 * Delay execution by a specified number of microseconds. On some system 
 *      this will not be at all accurate.
 */
void
xslDbgSleep(long delay)
{
#ifdef HAVE_USLEEP
    usleep(delay);
#else
#ifdef WIN32
    Sleep(delay / 1000);
#else
    /* try to delay things by doing a lot of floating point 
     * multiplication   
     */
    long loop1, loop2;
    float f1 = 1.0000001, f2;

    for (loop1 = 0; loop1 < 100000 * delay; loop1++)
        for (loop2 = 0; loop2 < 100000; loop2++) {
            f2 = f1 * f1;
        }
#endif
#endif
}


/**
 * xslDbgWalkContinue:
 *
 * Delay execution for time as indicated by OPTION_WALK_SPEED
 * Can only be called from within shellPrompt!
 * OPTION_WALK_SPEED != WALKSPEED_STOP
 *
 * Returns 1 if walk is to continue,
 *         0 otherwise
 */
int
xslDbgWalkContinue(void)
{
    int result = 0, speed = optionsGetIntOption(OPTIONS_WALK_SPEED);

    /* We need to ensure that output is realy sent. Otherwise
     * walking using xemacs under WIN32 will not work */
    fflush(stderr);

    switch (speed) {
        case WALKSPEED_1:
        case WALKSPEED_2:
        case WALKSPEED_3:
        case WALKSPEED_4:
        case WALKSPEED_5:
        case WALKSPEED_6:
        case WALKSPEED_7:
        case WALKSPEED_8:
        case WALKSPEED_9:
            /* see options.h for defintion of WALKDAY */
            xslDbgSleep(speed * WALKDELAY);
            result = 1;
            break;

        default:               /* stop walking */
            optionsSetIntOption(OPTIONS_WALK_SPEED, WALKSPEED_STOP);
            xslDebugStatus = DEBUG_STOP;
            break;
    }

    return result;
}


/**
 * addBreakPointNode:
 * @payload : valid breakPointPtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addBreakPointNode(void *payload, void *data,
                  xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node = searchBreakPointNode((breakPointPtr) payload);

    searchAdd(node);
}


/**
 * addSourceNode:
 * @payload : valid xsltStylesheetPtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addSourceNode(void *payload, void *data,
              xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node = searchSourceNode((xsltStylesheetPtr) payload);

    searchAdd(node);
}


/**
 * addTemplateNode:
 * @payload : valid xsltTemplatePtr
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addTemplateNode(void *payload, void *data,
                xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node =
        searchTemplateNode(((xsltTemplatePtr) payload)->elem);
    searchAdd(node);
}


/**
 * addGlobalNode:
 * @payload : valid xmlNodePtr of global variable
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addGlobalNode(void *payload, void *data,
              xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node = searchGlobalNode((xmlNodePtr) payload);

    searchAdd(node);
}


/**
 * addLocalNode:
 * @payload : valid xmlNodePtr of local variable
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addLocalNode(void *payload, void *data,
             xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node = searchLocalNode((xmlNodePtr) payload);

    searchAdd(node);
}


/**
 * addIncludeNode:
 * @payload : valid xmlNodePtr of include instuction
 * @data : not used
 * @name : not used
 * 
 * Convert payload into format needed, and add to search dataBase 
 */
void
addIncludeNode(void *payload, void *data,
               xmlChar * name)
{
    Q_UNUSED(data);
    Q_UNUSED(name);
    xmlNodePtr node = searchIncludeNode((xmlNodePtr) payload);

    searchAdd(node);
}


/**
 * addCallStackItems:
 * 
 * Convert call stack items into format needed, and add to search dataBase 
 */
void
addCallStackItems(void)
{
    callPointPtr item;
    xmlNodePtr node;
    int depth;

    for (depth = callStackGetDepth(); depth > 0; depth--) {
        item = callStackGet(depth);
        if (item) {
            node = searchCallStackNode(item);
            if (node)
                searchAdd(node);
        }
    }
}


/**
 * updateSearchData:
 * @styleCtxt: Not used
 * @style: Is valid
 * @data: Not used but MUST be NULL for the moment
 * @variableTypes: What types of variables to look
 *
 * Update the searchDatabase
 *
 * Returns 1 on success,
 *         0 otherwise
 */
int
updateSearchData(xsltTransformContextPtr styleCtxt,
                 xsltStylesheetPtr style,
                 void *data, VariableTypeEnum variableTypes)
{
    Q_UNUSED(styleCtxt);
    Q_UNUSED(variableTypes);
    int result = 0;

    if (!style) {
#ifdef WITH_XSLDBG_DEBUG_PROCESS
        xsltGenericError(xsltGenericErrorContext,
                         "Error: Unable to update search database no stylesheet loaded\n");
#endif
        return result;
    }
    searchEmpty();
    xsldbgGenericErrorFunc(i18n("Information: Updating search database. This may take a while...\n"));
    /* add items to the search dataBase */
    addCallStackItems();
    xsldbgGenericErrorFunc(i18n("Information: Looking for breakpoints.\n"));
    walkBreakPoints((xmlHashScanner) addBreakPointNode, data);
    xsldbgGenericErrorFunc(i18n("Information: Looking for imports and top-level stylesheets.\n"));
    walkStylesheets((xmlHashScanner) addSourceNode, data, style);
    xsldbgGenericErrorFunc(i18n("Information: Looking for xsl:includes.\n"));
    walkIncludeInst((xmlHashScanner) addIncludeNode, data, style);
    xsldbgGenericErrorFunc(i18n("Information: Looking for templates.\n"));
    walkTemplates((xmlHashScanner) addTemplateNode, data, style);
    xsldbgGenericErrorFunc(i18n("Information: Looking for global variables.\n"));
    walkGlobals((xmlHashScanner) addGlobalNode, data, style);
    xsldbgGenericErrorFunc(i18n("Information: Looking for local variables.\n"));
    walkLocals((xmlHashScanner) addLocalNode, data, style);
    xsldbgGenericErrorFunc(i18n("Information: Formatting output.\n"));

    result = searchSave(NULL);
    return result;
}


/**
 * debugXSLBreak:
 * @templ: The source node being executed
 * @node: The data node being processed
 * @root: The template being applied to "node"
 * @ctxt: The transform context for stylesheet being processed
 *
 * A break point has been found so pass control to user
 */
void
debugXSLBreak(xmlNodePtr templ, xmlNodePtr node, xsltTemplatePtr root,
              xsltTransformContextPtr ctxt)
{
    xmlDocPtr tempDoc = NULL;
    xmlNodePtr tempNode = NULL;
    rootCopy = root;
    static const xmlChar *lastTemplate = NULL;
    xmlBufferPtr buffer = xmlBufferCreate();
    static char mybuff[6] = " 0\n\x0";

    if (ctxt && templ && root && !xsldbgReachedFirstTemplate)
	xsldbgReachedFirstTemplate = true;

    if (templ == NULL) {
        tempDoc = xmlNewDoc((xmlChar *) "1.0");
        if (!tempDoc)
            return;
        tempNode = xmlNewNode(NULL, (xmlChar *) "xsldbg_default_node");
        if (!tempNode) {
            xmlFreeDoc(tempDoc);
            return;
        }
        xmlAddChild((xmlNodePtr) tempDoc, tempNode);
        templ = tempNode;
    }

    if (node == NULL)
        node = (xmlNodePtr) filesGetMainDoc();

    if (node == NULL) {
        tempDoc = xmlNewDoc((xmlChar *) "1.0");
        if (!tempDoc)
            return;
        tempNode = xmlNewNode(NULL, (xmlChar *) "xsldbg_default_node");
        if (!tempNode) {
            xmlFreeDoc(tempDoc);
            return;
        }
        xmlAddChild((xmlNodePtr) tempDoc, tempNode);
        node = tempNode;
    }
    if (root) {
        xmlChar *nameTemp = NULL, *modeTemp = NULL;
	nameTemp = fullQName(root->nameURI, root->name);
	modeTemp = fullQName(root->modeURI, root->mode);
	if (!nextCommandActive){
	  /* we only want messages if we are not
	    in the process of completing the next command */
	  if (terminalIO == NULL) {

            if (root->match){
                xsldbgGenericErrorFunc(i18n("\nReached template: \"%1\" mode: \"%2\"\n").arg(xsldbgText(root->match)).arg(xsldbgText(modeTemp)));
		if (lastTemplate != root->match && buffer){
		    xmlBufferCCat(buffer, "\nreached matched template:");
		    xmlBufferCat(buffer, root->match); 
		    xmlBufferCCat(buffer, mybuff);
		    xsltCopyTextString(ctxt, ctxt->insert,xmlBufferContent(buffer),0);
		    mybuff[1]++;
		    lastTemplate = root->match;
		}
            }else{
                xsldbgGenericErrorFunc(i18n("\nReached template: \"%1\" mode: \"%2\"\n").arg(xsldbgText(nameTemp)).arg(xsldbgText(modeTemp)));
		if (lastTemplate != root->name && buffer){
		    xmlBufferCCat(buffer, "\nreached named template:");
		    xmlBufferCat(buffer,root->match); 
		    xmlBufferCCat(buffer,mybuff);
		    xsltCopyTextString(ctxt, ctxt->insert,xmlBufferContent(buffer),0);
		    mybuff[1]++;
		    lastTemplate = root->name;
		}
		
	    }
	    if (buffer)
	       xmlBufferFree(buffer);
	  } else {
            if ((xslDebugStatus == DEBUG_TRACE) ||
                (xslDebugStatus == DEBUG_WALK)) {
		QString message;
                if (root->match)
		  message = i18n("\nReached template: \"%1\" mode: \"%2\"\n").arg(xsldbgText(root->match)).arg(xsldbgText(modeTemp));
                else
		  message = i18n("\nReached template: \"%1\" mode: \"%2\"\n").arg(xsldbgText(nameTemp)).arg(xsldbgText(modeTemp));
		fprintf(terminalIO, "%s", message.local8Bit().data());
            }
	  }
	}
	if (nameTemp)
	  xmlFree(nameTemp);
	if (modeTemp)
	  xmlFree(modeTemp);
    }

    shellPrompt(templ, node, (xmlChar *) "index.xsl",
                (xmlShellReadlineFunc) xslDbgShellReadline, stdout, ctxt);
    if (tempDoc)
        xmlFreeDoc(tempDoc);
}


/* Highly modified function based on xmlShell */

/**
 *  shellPrompt:
 * @source: The current stylesheet instruction being executed
 * @doc: The current document node being processed
 * @filename: Not used
 * @input: The function to call to when reading commands from stdio
 * @output: Where to put the results
 * @styleCtxt: Is valid 
 *
 * Present to the user the xsldbg shell
 */
void
shellPrompt(xmlNodePtr source, xmlNodePtr doc, xmlChar * filename,
            xmlShellReadlineFunc input, FILE * output,
            xsltTransformContextPtr styleCtxt)
{
    xmlChar prompt[DEBUG_BUFFER_SIZE] = "/ > ";
    xmlChar *cmdline = NULL, *cur;
    int nbargs = 0;
    int loadedFiles = 0;
    int commandId = -1;         /* stores what was the last 
                                 * command id entered  by user */
    xmlChar command[DEBUG_BUFFER_SIZE]; /* holds the command user entered */
    xmlChar arg[DEBUG_BUFFER_SIZE];     /* holds any extra arguments to
                                         * command entered */
    xmlChar dir[DEBUG_BUFFER_SIZE];     /* temporary buffer used by where
                                         * and pwd commands */
    int cmdResult;              /* result of last command */
    int shortCutId = -1;        /* used by cd command */
    int i;
    static int showSource = 1;  /* Do we first show source or data ? */
    xmlChar *baseUri = NULL;    /* for used for included xml entities */
    const xmlChar *breakUri;

    /* for convenience keep track of which node was last 
     * selected of source and doc */
    xmlNodePtr lastSourceNode, lastDocNode;


    xmlShellCtxtPtr ctxt;
    int exitShell = 0;          /* Indicate when to exit xslShell */

    if (source == NULL){
#ifdef WITH_XSLDBG_DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
			 "Error: Source NULL in shellPrompt\n");
#endif
        return;
    }
    if (doc == NULL){
#ifdef WITH_XSLDBG_DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
			 "Error: doc NULL in shellPrompt\n");
#endif
        return;
    }
    if (filename == NULL){
#ifdef WITH_XSLDBG_DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
			 "Error: fileName NULL in shellPrompt\n");
#endif
        return;
    }
    if (input == NULL){
#ifdef WITH_XSLDBG_DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
			 "Error: Input function NULL in shellPrompt\n");
#endif
        return;
    }
    if (output == NULL){
#ifdef WITH_XSLDBG_DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
			 "Error: Output NULL in shellPrompt\n");
#endif
        return;
    }
    ctxt = (xmlShellCtxtPtr) xmlMalloc(sizeof(xmlShellCtxt));
    if (ctxt == NULL){
	xsldbgGenericErrorFunc(i18n("Error: Out of memory.\n"));
        return;
    }

    /* flag that we've received control */
    debugGotControl(1);
    ctxt->loaded = 0;
    lastSourceNode = source;
    lastDocNode = doc;
    /* show the doc or source first? */
    if (showSource) {
        ctxt->doc = source->doc;
        ctxt->node = source;
        xsldbgUpdateFileDetails(source);
    } else {
        ctxt->doc = doc->doc;
        ctxt->node = (xmlNodePtr) doc;
        xsldbgUpdateFileDetails((xmlNodePtr) doc);
    }

    ctxt->input = input;
    ctxt->output = output;
    ctxt->filename = (char *) xmlStrdup((xmlChar *) filename);


    if (xsldbgStop == 1){
        xslDebugStatus = DEBUG_STOP;
	optionsSetIntOption(OPTIONS_TRACE,  TRACE_OFF);
        optionsSetIntOption(OPTIONS_WALK_SPEED, WALKSPEED_STOP);
	xsldbgStop = 0;
    }

    /* let any listener know that we got to a new line */
    if (xslDebugStatus != DEBUG_TRACE) {
        /* don't send notify message if we are tracing stylesheet */
        int breakpoint = 1;

        notifyXsldbgApp(XSLDBG_MSG_LINE_CHANGED, &breakpoint);
    }


    /* If using a thread and the thread is running then we don't need to 
     * send this as the application will see the XSLDBG_MSG_LINE_CHANGED message */
    if ((getThreadStatus() == XSLDBG_MSG_THREAD_NOTUSED) ||
        (xslDebugStatus == DEBUG_TRACE)) {
	QString messageTxt;
        if (!nextCommandActive && ctxt->node && ctxt->node && ctxt->node->doc
            && ctxt->node->doc->URL) {
            if (!showSource) {
                baseUri = filesGetBaseUri(ctxt->node);
                if (baseUri != NULL)
                    breakUri = baseUri;
                else
                    breakUri = ctxt->node->doc->URL;
            } else
                breakUri = ctxt->node->doc->URL;

            if (xmlGetLineNo(ctxt->node) != -1)
		messageTxt = i18n("Breakpoint for file \"%1\" at line %2.\n").arg(xsldbgUrl(breakUri)).arg(xmlGetLineNo(ctxt->node));
            else
                messageTxt = i18n("Breakpoint at text node in file \"%1\".\n").arg(xsldbgUrl(breakUri));
            if (baseUri != NULL) {
                xmlFree(baseUri);
                baseUri = NULL;
            }

            if (((xslDebugStatus == DEBUG_TRACE) ||
                 (xslDebugStatus == DEBUG_WALK)) && (terminalIO != NULL))
                fprintf(terminalIO, "%s", messageTxt.utf8().data());
            else
		xsldbgGenericErrorFunc(messageTxt);

        }
    }
    if ((showWatchesActive && (xslDebugStatus == DEBUG_TRACE)) || 
	(xslDebugStatus == DEBUG_WALK)){
      ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
      if (ctxt->pctxt) {
	xslDbgShellShowWatches(styleCtxt, ctxt, 0);
	xsldbgGenericErrorFunc("\n");
	xmlXPathFreeContext(ctxt->pctxt);
	ctxt->pctxt = NULL;
      }
    }

    if (xslDebugStatus == DEBUG_TRACE) {
	if (ctxt->filename)
	  xmlFree(ctxt->filename);
        xmlFree(ctxt);
        return; /* All done. Trace next instruction/node */
    } 
    if (xslDebugStatus == DEBUG_WALK) {
        if (xslDbgWalkContinue()) {
	  if (ctxt->filename)
	    xmlFree(ctxt->filename);
	  xmlFree(ctxt);
            return; /* All done. Walk to next instruction/node */
        }
    }

    ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
    if (ctxt->pctxt == NULL) {
        xmlFree(ctxt);
	xsldbgGenericErrorFunc(i18n("Error: Out of memory.\n"));
        return;
    }

    if (showWatchesActive){
      xslDbgShellShowWatches(styleCtxt, ctxt, 0);
      xsldbgGenericErrorFunc("\n");
    }

    while (!exitShell && (xslDebugStatus != DEBUG_QUIT)) {
        if (getThreadStatus() != XSLDBG_MSG_THREAD_RUN) {
            if (ctxt->node == (xmlNodePtr) ctxt->doc)
                snprintf((char *) prompt, DEBUG_BUFFER_SIZE - 1,
                         "(xsldbg) %s > ", "/");
            else if ((ctxt->node->name) && (ctxt->node->ns))
                snprintf((char *) prompt, DEBUG_BUFFER_SIZE - 1,
                         "(xsldbg) %s:%s > ", ctxt->node->ns->prefix,
                         ctxt->node->name);
            else if (ctxt->node->name)
                snprintf((char *) prompt, DEBUG_BUFFER_SIZE - 1,
                         "(xsldbg) %s > ", ctxt->node->name);

            else
                snprintf((char *) prompt, DEBUG_BUFFER_SIZE - 1,
                         "(xsldbg) ? > ");

            prompt[sizeof(prompt) - 1] = 0;

            /*
             * Get a new command line
             */
	    if (nextCommandActive && (xslDebugStatus == DEBUG_STEP))
	      /* we are processing the "next command" do the next 
	         part of the command which is the up command */
	      cmdline = xmlStrdup((xmlChar*)"up");
	    else
	      cmdline = (xmlChar *) ctxt->input((char *) prompt);
            if (cmdline && (optionsGetIntOption(OPTIONS_UTF8_INPUT) == 0)) {
                /* we are getting encoded characters from the command line
                 * so decode them into UTF-8 */
                xmlChar *tempResult = filesDecode(cmdline);

                if (tempResult) {
                    xmlFree(cmdline);
                    cmdline = tempResult;
                }
            }
        } else {
            /* don't need a prompt for running as when running as a thread */
            xmlStrCpy(messageBuffer, "");
	    if (nextCommandActive && (xslDebugStatus == DEBUG_STEP))
	      /* we are processing the "next command" do the next 
	         part of the command which is the up command */
	      cmdline = xmlStrdup((xmlChar*)"up");
	    else
	      cmdline = (xmlChar *) ctxt->input((char *) messageBuffer);
        }

        if (cmdline == NULL)
            break;

	/* don't allow next command to be active more than at one breakpoint */
	if (nextCommandActive)
	  nextCommandActive = 0;

        notifyXsldbgApp(XSLDBG_MSG_PROCESSING_INPUT, NULL);

        /* remove leading/trailing blanks */
        trimString(cmdline);

        /*
         * Parse the command itself
         */
        cur = cmdline;
        nbargs = 0;
        while ((*cur == ' ') || (*cur == '\t'))
            cur++;
        i = 0;
        while ((*cur != ' ') && (*cur != '\t') &&
               (*cur != '\n') && (*cur != '\r')) {
            if (*cur == 0)
                break;
            command[i++] = *cur++;
        }
        command[i] = 0;
        if (i == 0)
            continue;
        nbargs++;

        /*
         * Parse the argument
         */
        while ((*cur == ' ') || (*cur == '\t'))
            cur++;
        i = 0;
        while ((*cur != '\n') && (*cur != '\r') && (*cur != 0)) {
            if (*cur == 0)
                break;
            arg[i++] = *cur++;
        }
        arg[i] = 0;
        if (i != 0)
            nbargs++;


        commandId = lookupName(command, (xmlChar **) commandNames);
        /* try command shorts if command is not found */
        if (commandId < 0)
            commandId =
                lookupName(command, (xmlChar **) shortCommandNames);

        cmdResult = -1;         /* flag that it contains no result */
        /*
         * start interpreting the command
         */
        switch (commandId + DEBUG_HELP_CMD) {
                /* --- Help related commands --- */
            case DEBUG_HELP_CMD:
                cmdResult = helpTop(arg);
		if (!cmdResult)
		    xsldbgGenericErrorFunc(i18n("Unable to print local help. Online help can be found at http://xsldbg.sourceforge.net/docs/index.html.\n"));	    
                break;


                /* --- Running related commands --- */
            case DEBUG_BYE_CMD:
            case DEBUG_EXIT_CMD:
            case DEBUG_QUIT_CMD:
                /* allow the stylesheet to exit */
                xslDebugStatus = DEBUG_QUIT;
                exitShell++;
                cmdResult = 1;
                break;

	   case DEBUG_NEXT_CMD:
                xslDebugStatus = DEBUG_STEP;
                exitShell++;
                cmdResult = 1;
	        /* Do the the next part of this command
		   which is the up command */
		nextCommandActive = 1;
	     break;

            case DEBUG_STEP_CMD:
                xslDebugStatus = DEBUG_STEP;
                exitShell++;
                cmdResult = 1;
                break;

            case DEBUG_STEPUP_CMD:
                {
                    xmlChar *noOfFrames = arg;

                    /* skip until next space character */
                    while (*noOfFrames && (*noOfFrames != ' ')) {
                        noOfFrames++;
                    }
                    cmdResult = xslDbgShellFrameBreak(noOfFrames, 1);
                    exitShell++;
                }
                break;

            case DEBUG_STEPDOWN_CMD:
                {
                    xmlChar *noOfFrames = arg;

                    /* skip until next space character */
                    while (*noOfFrames && (*noOfFrames != ' ')) {
                        noOfFrames++;
                    }
                    cmdResult = xslDbgShellFrameBreak(noOfFrames, 0);
                    exitShell++;
                }
                break;

                /* continue to next break point */
            case DEBUG_CONT_CMD:
                xslDebugStatus = DEBUG_CONT;
                exitShell++;
                cmdResult = 1;
                break;

                /* restart */
            case DEBUG_RUN_CMD:
                xslDebugStatus = DEBUG_RUN_RESTART;
                exitShell++;
                cmdResult = 1;
                break;


                /* --- Template related commands --- */
            case DEBUG_TEMPLATES_CMD:
                {
                    int allFiles = 1, verbose = 1;

                    if (xmlStrLen(arg) && (xmlStrCmp(arg, "this") == 0)) {
                        allFiles = 0;
                    }

                    /* be verbose when printing template names */
                    /* if args is not empty then print names this stylesheet */
                    cmdResult =
                        xslDbgShellPrintTemplateNames(styleCtxt, ctxt, arg,
                                                      verbose, allFiles);
                    break;
                }


            case DEBUG_WHERE_CMD:
                /* Print the current working directory as well */
                xslDbgPrintCallStack(NULL);
                if (getThreadStatus() != XSLDBG_MSG_THREAD_RUN) {
                    if (!xmlShellPwd(ctxt, (char *) dir, ctxt->node, NULL)){
                        xsldbgGenericErrorFunc((const char*)dir);
                        xsldbgGenericErrorFunc("\n");
		    }
                }
                cmdResult = 1;

                break;

            case DEBUG_FRAME_CMD:
                cmdResult = xslDbgPrintCallStack(arg);
                break;

            case DEBUG_STYLESHEETS_CMD:
                cmdResult = xslDbgShellPrintStyleSheets(arg);
                break;

                /* --- Break point related commands --- */
            case DEBUG_BREAK_CMD:
                if (xmlStrLen(arg)) {
                    if (styleCtxt)
                        cmdResult =
                            xslDbgShellBreak(arg, styleCtxt->style,
                                             styleCtxt);
                    else
                        cmdResult = xslDbgShellBreak(arg, NULL, styleCtxt);
                } else {
                    /* select current node to break at */
		    xmlChar buff[100];
		    xmlChar *tempBaseName = filesGetBaseUri(ctxt->node);		    
		    if (tempBaseName){
		      snprintf((char *) buff, sizeof(buff), "-l %s %ld",
			       tempBaseName,
			       xmlGetLineNo(ctxt->node));
		      xmlFree(tempBaseName);
		    }
                    if (styleCtxt)
                        cmdResult =
                            xslDbgShellBreak(buff, styleCtxt->style,
                                             styleCtxt);
                    else
                        cmdResult =
                            xslDbgShellBreak(buff, NULL, styleCtxt);
                }

                break;

            case DEBUG_SHOWBREAK_CMD:
                if (getThreadStatus() == XSLDBG_MSG_THREAD_RUN) {
                    notifyListStart(XSLDBG_MSG_BREAKPOINT_CHANGED);
                    walkBreakPoints((xmlHashScanner)
                                    xslDbgShellPrintBreakPoint, NULL);
                    notifyListSend();
                } else {
                    xsldbgGenericErrorFunc("\n");
                    printCount = 0;     /* printCount will get updated by
                                         * xslDbgShellPrintBreakPoint */

                    walkBreakPoints((xmlHashScanner)
                                    xslDbgShellPrintBreakPoint, NULL);
                    if (printCount == 0)
                        xsldbgGenericErrorFunc(i18n("\nNo breakpoints are set for the file.\n"));
                    else
                        xsldbgGenericErrorFunc(i18n("\tTotal of %n breakpoint present.","\tTotal of %n breakpoints present.", printCount) + QString("\n"));
                }
                cmdResult = 1;
                break;

            case DEBUG_DELETE_CMD:
	      if (xmlStrLen(arg))
                    cmdResult = xslDbgShellDelete((xmlChar *) arg);
                else {
                    breakPointPtr breakPtr = NULL;
		    xmlChar* tempBaseName = filesGetBaseUri(ctxt->node);

                    if (tempBaseName){
                        breakPtr =
                            breakPointGet(tempBaseName,
					  xmlGetLineNo(ctxt->node));
			xmlFree(tempBaseName);
		    }
                    if (!breakPtr || !breakPointDelete(breakPtr)) {
                        xsldbgGenericErrorFunc(i18n("Error: Unable to delete breakpoint.\n"));
                        cmdResult = 0;
                    }
                }
                break;

            case DEBUG_ENABLE_CMD:
                if (xmlStrLen(arg))
                    cmdResult =
                        xslDbgShellEnable(arg, XSL_TOGGLE_BREAKPOINT);
                else {
                    breakPointPtr breakPtr = NULL;
		    xmlChar * tempBaseName = filesGetBaseUri(ctxt->node);

		    if (tempBaseName){
                        breakPtr =
                            breakPointGet(tempBaseName,
					  xmlGetLineNo(ctxt->node));
		      xmlFree(tempBaseName);
		    }
                    if (!breakPtr ||
                        (!breakPointEnable(breakPtr, !(breakPtr->flags & BREAKPOINT_ENABLED)))) {
                        xsldbgGenericErrorFunc(i18n("Error: Unable to enable/disable breakpoint.\n"));
                        cmdResult = 0;
                    }
                }
                break;

            case DEBUG_DISABLE_CMD:
	      if (xmlStrLen(arg))
                    cmdResult = xslDbgShellEnable(arg, 0);
                else {
                    breakPointPtr breakPtr = NULL;
		    xmlChar *tempBaseName = filesGetBaseUri(ctxt->node);
		      
		    if (tempBaseName){
                        breakPtr =
			  breakPointGet(tempBaseName,
                                          xmlGetLineNo(ctxt->node));
		      xmlFree(tempBaseName);
		    }
                    if (!breakPtr || !breakPointEnable(breakPtr, 0)) {
                        xsldbgGenericErrorFunc(i18n("Error: Unable to enable/disable breakpoint.\n"));
                        cmdResult = 0;
                    }
                }
                break;



                /* --- Node view related commands --- */
            case DEBUG_LS_CMD:
                cmdResult = xslDbgShellPrintList(ctxt, arg, 0);
                break;

            case DEBUG_DIR_CMD:
                cmdResult = xslDbgShellPrintList(ctxt, arg, 1);
                break;

            case DEBUG_DU_CMD:
                xmlShellDu(ctxt, NULL, ctxt->node, NULL);
                cmdResult = 1;
                break;

            case DEBUG_CAT_CMD:
            case DEBUG_PRINT_CMD:
                cmdResult = xslDbgShellCat(styleCtxt, ctxt, arg);
                break;

            case DEBUG_PWD_CMD:
                if (!xmlShellPwd(ctxt, (char *) dir, ctxt->node, NULL)) {
		  xmlChar* tempBaseName = filesGetBaseUri(ctxt->node);
		  if(tempBaseName){
		    xsldbgGenericErrorFunc("\n");
		    xsldbgGenericErrorFunc((char*)dir);
		    xsldbgGenericErrorFunc(i18n(" in file \"%1\" at line %2").arg(xsldbgUrl(tempBaseName)).arg(xmlGetLineNo(ctxt->node)));
		    xmlFree(tempBaseName);
		    cmdResult = 1;
		  }
                }
		if (cmdResult)
		  xsldbgGenericErrorFunc("\n");
		else
		  xsldbgGenericErrorFunc(i18n("Error: Unable to print working directory.\n"));		  
                break;

            case DEBUG_DUMP_CMD:
                xmlDebugDumpDocument(stdout, ctxt->doc);
                cmdResult = 1;
                break;

            case DEBUG_BASE_CMD:
                xmlShellBase(ctxt, NULL, ctxt->node, NULL);
                cmdResult = 1;
                break;


                /* ---  Variable related commands --- */
            case DEBUG_GLOBALS_CMD:
                if (loadedFiles == 0)
                    cmdResult = xslDbgShellPrintVariable(styleCtxt, arg,
                                                         DEBUG_GLOBAL_VAR);
                else {
                    xsldbgGenericErrorFunc(i18n("Error: Need to use the run command first.\n"));
                    cmdResult = 0;
                }
                break;

            case DEBUG_LOCALS_CMD:
                if (loadedFiles == 0)
                    /* if gdb compatability mode is enable print the globals then 
                     * the locals */
                    if (optionsGetIntOption(OPTIONS_GDB) == 1) {
                        cmdResult =
                            xslDbgShellPrintVariable(styleCtxt, arg,
                                                     DEBUG_GLOBAL_VAR);
                        if (cmdResult == 1)
                            cmdResult =
                                xslDbgShellPrintVariable(styleCtxt, arg,
                                                         DEBUG_LOCAL_VAR);
                    } else
                        cmdResult =
                            xslDbgShellPrintVariable(styleCtxt, arg,
                                                     DEBUG_LOCAL_VAR);
                else {
                    xsldbgGenericErrorFunc(i18n("Error: Need to use the run command first.\n"));
                    cmdResult = 0;
                }
                break;


                /* It's difficult to put the following commands into 
                 * a separe file so they stay here! */
                /* --- Node selection related commands --- */
            case DEBUG_SOURCE_CMD:
                cmdResult = 1;  /* only one case where this will command fail */
		xsldbgValidateBreakpoints = BREAKPOINTS_NEED_VALIDATION;
                if (xmlStrLen(arg) == 0) {
                    if (ctxt->doc == doc->doc)
                        lastDocNode = ctxt->node;
                    ctxt->doc = source->doc;
                    ctxt->node = lastSourceNode;
		    if (ctxt->pctxt)
		      xmlXPathFreeContext(ctxt->pctxt);
                    ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
                    showSource = 1;
                    xsldbgUpdateFileDetails((xmlNodePtr) ctxt->node);
                    /* let any listener know that we got to a new line */
                    notifyXsldbgApp(XSLDBG_MSG_LINE_CHANGED, NULL);
                    if (ctxt->pctxt == NULL) {
                        xmlFree(ctxt);
                        xslDebugStatus = DEBUG_QUIT;
                        return;
                    } else
                        break;
                } else {
                    /* load new stylesheet file, actual loading happens later */
                    xmlChar *expandedName = filesExpandName(arg);

                    if (expandedName) {
                        xsldbgGenericErrorFunc(i18n("Load of source deferred. Use the run command.\n"));

                        optionsSetStringOption(OPTIONS_SOURCE_FILE_NAME,
                                               expandedName);
			notifyXsldbgApp(XSLDBG_MSG_FILE_CHANGED, 0L);
                        loadedFiles = 1;
                        xmlFree(expandedName);
                        cmdResult = 1;
                    } else {
                        cmdResult = 0;
                    }
                }
                break;

            case DEBUG_DATA_CMD:
                cmdResult = 1;  /* only one case where this will command fail */
		xsldbgValidateBreakpoints = BREAKPOINTS_NEED_VALIDATION;
                if (xmlStrLen(arg) == 0) {
                    if (ctxt->doc == source->doc)
                        lastSourceNode = ctxt->node;
                    ctxt->doc = doc->doc;
                    ctxt->node = lastDocNode;
		    if (ctxt->pctxt)
		      xmlXPathFreeContext(ctxt->pctxt);
                    ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
                    showSource = 0;
                    xsldbgUpdateFileDetails((xmlNodePtr) ctxt->node);
                    /* let any listener know that we got to a new line */
                    notifyXsldbgApp(XSLDBG_MSG_LINE_CHANGED, NULL);
                    if (ctxt->pctxt == NULL) {
                        xmlFree(ctxt);
                        xslDebugStatus = DEBUG_QUIT;
                        return;
                    } else
                        break;
                } else {
                    /* load new xml file actual loading hapens later */
                    xmlChar *expandedName = filesExpandName(arg);

                    if (expandedName) {
                        xsldbgGenericErrorFunc(i18n("Load of data file deferred. Use the run command.\n"));

                        optionsSetStringOption(OPTIONS_DATA_FILE_NAME,
                                               expandedName);
			notifyXsldbgApp(XSLDBG_MSG_FILE_CHANGED, 0L);
                        loadedFiles = 1;
                        xmlFree(expandedName);
                        cmdResult = 1;
                    } else {
                        cmdResult = 0;
                    }
                }
                break;

            case DEBUG_OUTPUT_CMD:
	      /* set the output file name to use */
	      cmdResult = xslDbgShellOutput(arg);
                break;

            case DEBUG_CD_CMD:
                /* use dir as a working buffer */
                xmlStrnCpy(dir, arg, 2);
                dir[2] = '\0';
                shortCutId = lookupName(dir, (xmlChar **) cdShortCuts);
                if (shortCutId >= 0) {
                    if (xmlStrLen(arg) == 2) {
                        cmdResult = xslDbgCd(styleCtxt, ctxt, (xmlChar *)
                                             cdAlternative[shortCutId],
                                             source);
                    } else {

                        xmlStrCpy(dir, cdAlternative[shortCutId]);
                        xmlStrCat(dir, &arg[2]);
                        cmdResult = xslDbgCd(styleCtxt, ctxt, dir, source);
                    }

                } else
                    cmdResult = xslDbgCd(styleCtxt, ctxt, arg, source);
                break;


                /* --- File related commands --- */
            case DEBUG_ENTITIES_CMD:
                cmdResult = xslDbgEntities();
                break;

            case DEBUG_SYSTEM_CMD:
	      /* strip off a single argument. I need to do it this
	         way because I've already public this API */
	      {
		xmlChar *systemID;
		if (splitString(arg, 1, &systemID) == 1){
		  cmdResult = xslDbgSystem(systemID);
		}else{
		  xsldbgGenericErrorFunc(i18n("Error: Invalid arguments for the command %1.\n").arg("system"));
		}
	       }
                break;

            case DEBUG_PUBLIC_CMD:
	      /* strip off a single argument. I need to do it this
	         way because I've already public this API */
	      {
		xmlChar *publicID;
		if (splitString(arg, 1, &publicID) == 1){
		  cmdResult = xslDbgPublic(publicID);
		}else{
		  xsldbgGenericErrorFunc(i18n("Error: Invalid arguments for the command %1.\n").arg("public"));
		}
	       }
                break;

            case DEBUG_ENCODING_CMD:
                cmdResult = xslDbgEncoding(arg);
                break;

            case DEBUG_VALIDATE_CMD:
                xsldbgGenericErrorFunc(i18n("Warning: The %1 command is disabled.\n").arg("validate"));
                cmdResult = 0;
                /*
                 * xmlShellValidate(ctxt, arg, NULL, NULL);
                 */
                break;

            case DEBUG_LOAD_CMD:
                cmdResult = optionsLoad();
                /* restart xsldbg and activate new configuration */
                if (cmdResult == 1) {
                    xslDebugStatus = DEBUG_RUN_RESTART;
                    exitShell++;
                }
                break;

            case DEBUG_SAVE_CMD:
                cmdResult = optionsSave();
                break;

            case DEBUG_WRITE_CMD:
                xsldbgGenericErrorFunc(i18n("Warning: The %1 command is disabled.\n").arg("write"));
                cmdResult = 0;
                /*
                 * xmlShellWrite(ctxt, arg, NULL, NULL);
                 */
                break;

            case DEBUG_FREE_CMD:
                xsldbgGenericErrorFunc(i18n("Warning: The %1 command is disabled.\n").arg("free"));
                cmdResult = 0;
                /*
                 * if (arg[0] == 0) {
                 * xmlMemShow(stdout, 0);
                 * } else {
                 * int len = 0;
                 * sscanf(arg, "%d", &len);
                 * xmlMemShow(stdout, len);
                 * }
                 */
                break;


                /* operating system related */
            case DEBUG_CHDIR_CMD:
                cmdResult = xslDbgShellChangeWd(arg);
                break;

            case DEBUG_SHELL_EXEC_CMD:
                cmdResult = xslDbgShellExecute(arg, 1);
                break;


                /* libxslt parameter related commands */
            case DEBUG_ADDPARAM_CMD:
                cmdResult = xslDbgShellAddParam(arg);
                break;

            case DEBUG_DELPARAM_CMD:
                cmdResult = xslDbgShellDelParam(arg);
                break;

            case DEBUG_SHOWPARAM_CMD:
                cmdResult = xslDbgShellShowParam(arg);
                break;



                /* option related commmands */
            case DEBUG_SETOPTION_CMD:
                cmdResult = xslDbgShellSetOption(arg);
                break;

            case DEBUG_OPTIONS_CMD:
                cmdResult = xslDbgShellOptions();
                break;



                /* misc commands */
            case DEBUG_TTY_CMD:
                if (openTerminal(arg)) {
		  /* gdb does to say anything after redirecting its 
		     output */
		  if (optionsGetIntOption(OPTIONS_GDB) < 2)
                    xsldbgGenericErrorFunc(i18n("Opening terminal %1.\n").arg(xsldbgText(arg)));
                    cmdResult = 1;
                } else
                    cmdResult = 0;
                break;


		/* language selection for messages */
	    case DEBUG_LANG_CMD:
#ifdef LOCALE_PREFIX
		if (xmlStrlen(arg) > 0){
		    setlocale(LC_MESSAGES, (char*)arg);
		    textdomain("kdewebdev/xsldsbg");
		    bindtextdomain("kdewebdev/xsldbg", LOCALE_PREFIX);
		    cmdResult = 1;
		}else{
		    xsldbgGenericErrorFunc(i18n("Error: Missing arguments for the command %1.\n").arg("lang"));
		    cmdResult = 0;
		}
#else
	        xsldbgGenericErrorFunc(i18n("Warning: The %1 command is disabled\n").arg("lang"));
		cmdResult = 1;
#endif
		break;

                /* tracing related commands */
            case DEBUG_TRACE_CMD:
                if (xslDbgShellTrace(arg)) {
                    exitShell++;
                    cmdResult = 1;
                } else
                    cmdResult = 0;

                break;

            case DEBUG_WALK_CMD:
                if (xslDbgShellWalk(arg)) {
                    exitShell++;
                    cmdResult = 1;
                } else
                    cmdResult = 0;
                break;

	   case DEBUG_ADDWATCH_CMD:
	     cmdResult = xslDbgShellAddWatch(arg);
	   break;

	   case DEBUG_DELWATCH_CMD:
	     cmdResult = xslDbgShellDeleteWatch(arg);	     
           break;

	   case DEBUG_SHOWWATCH_CMD:
	     trimString(arg);
	     switch (arg[0]){
	     case '\0':
	       cmdResult = xslDbgShellShowWatches(styleCtxt, ctxt, 1);
	       break;

	     case '0':
	     case '1':
               showWatchesActive = arg[0] - '0';
	       cmdResult = 1;
	       break;

	     default:
	        xsldbgGenericErrorFunc(i18n("Error: Invalid arguments for the command %1.\n").arg("showmatch")); 
	     }
           break;
 

                /* search related commands */
            case DEBUG_SEARCH_CMD:
                cmdResult =
                    xslDbgShellSearch(styleCtxt, filesGetStylesheet(),
                                      arg);
                break;



            case DEBUG_SET_CMD:
                cmdResult = xslDbgShellSetVariable(styleCtxt, arg);
                break;

            default:
                xsldbgGenericErrorFunc(i18n("Error: Unknown command %1. Try help.\n").arg(xsldbgText(command)));
                cmdResult = 0;
        }

        /* KDbg likes to get the marker after every command so here it is */
        if ((optionsGetIntOption(OPTIONS_GDB) >1) && optionsGetIntOption(OPTIONS_VERBOSE) && !nextCommandActive
	     && (commandId != DEBUG_STEPUP_CMD - DEBUG_HELP_CMD)) {
            if (ctxt->node && ctxt->node && 
		ctxt->node->doc && ctxt->node->doc->URL) {

                if (xmlGetLineNo(ctxt->node) != -1)
                    xsldbgGenericErrorFunc(i18n("Breakpoint for file \"%1\" at line %2.\n").arg(xsldbgUrl(ctxt->node->doc->URL)).arg(xmlGetLineNo(ctxt->node)));
                else
                    xsldbgGenericErrorFunc(i18n("Breakpoint at text node in file \"%1\".\n").arg(xsldbgUrl(ctxt->node->doc->URL)));
            }
        }

        /* notify any listeners of that the command failed */
        if (cmdResult == 0 && (xmlStrlen(messageBuffer) > 0)) {
            snprintf((char *) messageBuffer, sizeof(messageBuffer),
                     "\nRequest to xsldbg failed:%s\n", cmdline);
            notifyTextXsldbgApp(XSLDBG_MSG_TEXTOUT,
                                (char *) messageBuffer);
        }

        xmlFree(cmdline);
        cmdline = NULL;
    }

    xmlXPathFreeContext(ctxt->pctxt);

    if (ctxt->filename)
        xmlFree(ctxt->filename);

    xmlFree(ctxt);

    if (cmdline)
        xmlFree(cmdline);
}
