/***************************************************************************
                          listbox.h - Listbox widget 
                          -------------------
    copyright            : (C) 2002-2003 Marc Britton <consume@optusnet.com.au>
                           (C) 2004      Michal Rudolf <mrudolf@kdewebdev.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HAVE_LISTBOX_H_
#define _HAVE_LISTBOX_H_

/* KDE INCLUDES */

/* QT INCLUDES */
#include <qstringlist.h>
#include <qobject.h>
#include <qstring.h>

/* OTHER INCLUDES */
#include <kommanderwidget.h>
#include <klistbox.h>
#include <kommander_export.h>
class QWidget;

class QShowEvent;
class KOMMANDER_EXPORT ListBox : public KListBox, public KommanderWidget
{
  Q_OBJECT
  
  Q_PROPERTY(QString populationText READ populationText WRITE setPopulationText DESIGNABLE false)
  Q_PROPERTY(QStringList associations READ associatedText WRITE setAssociatedText DESIGNABLE false)
  Q_PROPERTY(bool KommanderWidget READ isKommanderWidget)
public:
  ListBox(QWidget *a_parent, const char *a_name);
  ~ListBox();
  
  virtual bool isKommanderWidget() const;
  virtual void setAssociatedText(const QStringList&);
  virtual QStringList associatedText() const;
  virtual QString currentState() const;
  virtual QString populationText() const;
  virtual void setPopulationText(const QString&);

  virtual QString handleDCOP(int function, const QStringList& args);    
  virtual bool isFunctionSupported(int function);
public slots:
  virtual void setWidgetText(const QString&);
  virtual void populate();
signals:
  void widgetOpened();
  void widgetTextChanged(const QString&);
  void contextMenuRequested(int xpos, int ypos);
protected:
  void showEvent(QShowEvent *e);
  void contextMenuEvent( QContextMenuEvent * e );
private:
};

#endif
