/* @(#$Id: uiprefs_w.h,v 1.1 2006-09-04 15:13:02 dockes Exp $  (C) 2006 J.F.Dockes */
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
#ifndef _UIPREFS_W_H_INCLUDED_
#define _UIPREFS_W_H_INCLUDED_
/* @(#$Id: uiprefs_w.h,v 1.1 2006-09-04 15:13:02 dockes Exp $  (C) 2005 J.F.Dockes */
#include <qvariant.h>
#include <qdialog.h>

#include "uiprefs.h"

class UIPrefsDialog : public UIPrefsDialogBase
{
    Q_OBJECT

public:
    UIPrefsDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 ): UIPrefsDialogBase(parent,name,modal,fl) {init();}
	~UIPrefsDialog(){};

    QString reslistFontFamily;
    int reslistFontSize;

    virtual void init();

public slots:
    virtual void showFontDialog();
    virtual void resetReslistFont();
    virtual void showBrowserDialog();
    virtual void extraDbTextChanged( const QString & text );
    virtual void addAADbPB_clicked();
    virtual void addADbPB_clicked();
    virtual void delADbPB_clicked();
    virtual void delAADbPB_clicked();
    virtual void addExtraDbPB_clicked();
    virtual void browseDbPB_clicked();

signals:
    void uiprefsDone();

protected slots:
    virtual void accept();
};


#endif /* _UIPREFS_W_H_INCLUDED_ */
