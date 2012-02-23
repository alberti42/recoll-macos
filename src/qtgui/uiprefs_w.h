/* Copyright (C) 2006 J.F.Dockes 
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
#include <qvariant.h>
#include <qdialog.h>

#include "ui_uiprefs.h"

class QDialog;
class ViewAction;

class UIPrefsDialog : public QDialog, public Ui::uiPrefsDialogBase
{
    Q_OBJECT

public:
    UIPrefsDialog(QDialog* parent = 0)
	: QDialog(parent) 
	{
	    setupUi(this);
	    init();
	}
	~UIPrefsDialog(){};

    QString reslistFontFamily;
    int reslistFontSize;
    QString stylesheetFile;

    virtual void init();

public slots:
    virtual void showFontDialog();
    virtual void resetReslistFont();
    virtual void showStylesheetDialog();
    virtual void resetStylesheet();
    virtual void showViewAction();
    virtual void showViewAction(const QString& mt);
    virtual void addExtraDbPB_clicked();
    virtual void delExtraDbPB_clicked();
    virtual void togExtraDbPB_clicked();
    virtual void actAllExtraDbPB_clicked();
    virtual void unacAllExtraDbPB_clicked();
    virtual void setStemLang(const QString& lang);
    virtual void editParaFormat();
    virtual void editHeaderText();

signals:
    void uiprefsDone();

protected slots:
    virtual void accept();
    virtual void reject();
private:
    // Locally stored data (pending ok/cancel)
    QString paraFormat;
    QString headerText;
    void setFromPrefs();
    ViewAction *m_viewAction;

};

#endif /* _UIPREFS_W_H_INCLUDED_ */
