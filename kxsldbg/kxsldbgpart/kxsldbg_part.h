#ifndef KXSLDBGKPART_H
#define KXSLDBGKPART_H

#include <kparts/part.h>
#include <qvariant.h>
#include <qdict.h>
#include <qmainwindow.h>
#include "xsldbgdebugger.h"
#include "xsldbginspector.h"
#include <dcopclient.h>
#include "kxsldbg_partif.h"
#include <ktexteditor/document.h> 
#include <ktexteditor/view.h> 

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QAction;
class QActionGroup;
class QToolBar;
class QPopupMenu;
class QTextBrowser;
class QLineEdit;
class QDockWindow;
class QWidgetStack;

class XsldbgBrkStatusView;
class QXsldbgDoc;
class XsldbgOutputView;
class XsldbgConfigImpl;

class QWidget;
class QPainter;
class KURL;
class QMultiLineEdit;
class KAboutData;

typedef QDict<QXsldbgDoc> XsldbgDocDict;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Keith Isdale <k_isdale@tpg.com.au>
 * @version 0.1
 */
class KXsldbgPart : public KParts::ReadOnlyPart, public KXsldbgPartIf 
{
    Q_OBJECT

public:
    /**
     * Default constructor
     */
    KXsldbgPart(QWidget *parentWidget, const char *widgetName,
                    QObject *parent, const char *name, const QStringList &args);

    /**
     * Destructor
     */
    virtual ~KXsldbgPart();

    static KAboutData *createAboutData();


   /** Checks the debugger, if it is not ready then
     *      display a error in a message box
     *
     * @returns TRUE if debugger is ready, otherwise FALSE
     */
    bool checkDebugger();
    void lookupSystemID(QString systemID);
    void lookupPublicID(QString publicID);
    void createInspector();

public slots:
    virtual bool openURL(const KURL &url);
    virtual bool closeURL();
    void quit();
    void emitOpenFile(QString file, int line, int row);
    void slotLookupSystemID();
    void slotLookupPublicID();
    void walkCmd_activated();
    void walkStopCmd_activated();
    void traceCmd_activated();
    void traceStopCmd_activated();
    void configureEditorCmd_activated(); 

  //Tool bar commands
    void configureCmd_activated();
    void inspectorCmd_activated();
    void runCmd_activated();
    void stepCmd_activated();
    void nextCmd_activated();
    void continueCmd_activated();
    void stepupCmd_activated();
    void stepdownCmd_activated();
    void sourceCmd_activated();
    void dataCmd_activated();
    void outputCmd_activated();
    void refreshCmd_activated();
    void enableCmd_activated();
    void breakCmd_activated();
    void deleteCmd_activated();

    /** Evaluate expression entered */	
    void evaluateCmd_activated();

    /** Goto/display file XPath entered */
    void gotoXPathCmd_activated();

    virtual void lineNoChanged( QString fileName, int lineNumber,
				bool breakpoint );

    void addBreakPoint(int lineNumber);
    void enableBreakPoint(int lineNumber);
    void deleteBreakPoint(int lineNumber);

    /** Evaluate expression entered in expressionEdit */
    void slotEvaluate();

    /** Goto/display file that matches XPath specified in xPathEdit */
    void slotGotoXPath();

     /** Run search on data base , see search.dtd for DTD of search database */
    void slotSearch();

    /* Process the the URI for SystemID or PublicID requested */
    void slotProcResolveItem(QString URI);

    /* used to get breakpoint list notfication */
    void breakpointItem(QString fileName, int  lineNumber ,
			QString /*templateName*/, QString /* modeName */,
			bool  enabled , int /* id */);

    void cursorPositionChanged();
    void docChanged();
    
    void debuggerStarted();

protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();
    bool fetchURL(const KURL &url);

protected slots:
    void fileOpen();

private:
    QXsldbgDoc *currentDoc;
    QWidgetStack *mainView;

    QPushButton *xPathBtn, *searchBtn, *evaluateBtn;
    QLineEdit *newXPath, *newSearch, *newEvaluate;
    QGridLayout* qxsldbgLayout;

    uint currentLineNo, currentColumnNo;
    XsldbgDocDict docDictionary;
    XsldbgInspector *inspector;
    XsldbgDebugger *debugger;
    XsldbgConfigImpl *configWidget;
    QString currentFileName;

    XsldbgOutputView *outputview;
};

#endif // KXSLDBGPART_H
