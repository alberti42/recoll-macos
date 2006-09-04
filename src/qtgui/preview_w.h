#ifndef _PREVIEW_W_H_INCLUDED_
#define _PREVIEW_W_H_INCLUDED_
/* @(#$Id: preview_w.h,v 1.1 2006-09-04 15:13:01 dockes Exp $  (C) 2006 J.F.Dockes */
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qvariant.h>
#include <qwidget.h>
#include "rcldb.h"
#include "preview.h"

class Preview : public PreviewBase
{
    Q_OBJECT

public:
    Preview( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 ) :
	PreviewBase(parent,name,fl) {init();}
	
    ~Preview(){}

    virtual void closeEvent( QCloseEvent * e );
    virtual bool eventFilter( QObject * target, QEvent * event );
    virtual bool makeDocCurrent( const string & fn, const Rcl::Doc & doc );
    virtual QTextEdit * getCurrentEditor();
    virtual QTextEdit * addEditorTab();
    virtual bool loadFileInCurrentTab(string fn, size_t sz, 
				      const Rcl::Doc & idoc );

public slots:
    virtual void searchTextLine_textChanged( const QString & text );
    virtual void doSearch( const QString &str, bool next, bool reverse );
    virtual void nextPressed();
    virtual void prevPressed();
    virtual void currentChanged( QWidget * tw );
    virtual void closeCurrentTab();
    virtual void setCurTabProps( const string & fn, const Rcl::Doc & doc );
    virtual void textDoubleClicked(int, int);

signals:
    void previewClosed(QWidget *);
    void wordSelect(QString);

protected:
    int matchIndex;
    int matchPara;
    bool dynSearchActive;
    bool canBeep;
    void *tabData;
    QWidget *currentW;

private:
    void init();
    virtual void destroy();
};

#endif /* _PREVIEW_W_H_INCLUDED_ */
