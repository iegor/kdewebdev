/*************************************************************************** 
                          xsldbglistitem.cpp  -  description 
                             ------------------- 
    begin                : Sun Jan 6 2002 
    copyright            : (C) 2002 by Andras Mantia 
    email                : amantia@kde.org 
 ***************************************************************************/ 
 
/*************************************************************************** 
 *                                                                         * 
 *   This program is free software; you can redistribute it and/or modify  * 
 *   it under the terms of the GNU General Public License as published by  * 
 *   the Free Software Foundation; either version 2 of the License, or     * 
 *   (at your option) any later version.                                   * 
 *                                                                         * 
 ***************************************************************************/ 
 
#include "xsldbglistitem.h" 
 
XsldbgListItem::XsldbgListItem(QListView *parent, int columnOffset, 
	const QString & fileName, int lineNumber) 
	: QListViewItem(parent) 
{ 
	this->fileName = fileName; 
	this->lineNumber = lineNumber; 
	setText(columnOffset, fileName); 
	/* we may not have a line number column so skip it if needed */ 
	if ((listView()->columns() >= columnOffset + 1) && (lineNumber != -1)) 
		setText(columnOffset + 1, QString::number(lineNumber)); 
} 
 
XsldbgListItem::~XsldbgListItem() 
{ 
} 
 
 
QString XsldbgListItem::getFileName() const 
{ 
	return fileName; 
} 
 
int XsldbgListItem::getLineNumber() const
{ 
	return lineNumber; 
} 
 
