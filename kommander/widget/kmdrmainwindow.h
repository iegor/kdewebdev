//
// C++ Interface: KmdrMainWindow
//
// Description: 
//
//
// Author: Andras Mantia <amantia@kdewebdev.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KmdrMainWindow_H
#define KmdrMainWindow_H

#include <kmainwindow.h>
#include <kommander_export.h>

/**
	@author Andras Mantia <amantia@kdewebdev.org>
*/
class KOMMANDER_EXPORT KmdrMainWindow : public KMainWindow
{
Q_OBJECT
public:
    KmdrMainWindow(QWidget *parent = 0, const char *name = 0, WFlags f = WType_TopLevel | WDestructiveClose) __attribute__(( visibility("default")));

    ~KmdrMainWindow();

protected:
    virtual bool queryClose();

signals:
    void initialize();
    void destroy();

};

#endif
