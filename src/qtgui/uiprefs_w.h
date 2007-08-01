/* @(#$Id: uiprefs_w.h,v 1.9 2007-08-01 07:55:03 dockes Exp $  (C) 2006 J.F.Dockes */
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
/* @(#$Id: uiprefs_w.h,v 1.9 2007-08-01 07:55:03 dockes Exp $  (C) 2005 J.F.Dockes */
#include <qvariant.h>
#include <qdialog.h>

#if (QT_VERSION < 0x040000)
#include "uiprefs.h"
#else
#include "ui_uiprefs.h"
#endif

class QDialog;

//MOC_SKIP_BEGIN
#if QT_VERSION < 0x040000
class DummyUIPrefsDialogBase : public UIPrefsDialogBase
{
public: DummyUIPrefsDialogBase(QWidget* parent = 0) 
    : UIPrefsDialogBase(parent) {}
};
#else
class DummyUIPrefsDialogBase : public QDialog, public Ui::UIPrefsDialogBase
{
 public: DummyUIPrefsDialogBase(QDialog *parent):QDialog(parent) {setupUi(this);}
};
#endif
//MOC_SKIP_END

class ViewAction;

class UIPrefsDialog : public DummyUIPrefsDialogBase
{
    Q_OBJECT

public:
    UIPrefsDialog(QDialog* parent = 0)
	: DummyUIPrefsDialogBase(parent) 
	{
	    init();
	}
	~UIPrefsDialog(){};

    QString reslistFontFamily;
    int reslistFontSize;

    virtual void init();

public slots:
    virtual void showFontDialog();
    virtual void showViewAction();
    virtual void resetReslistFont();
    virtual void showBrowserDialog();
    virtual void extraDbTextChanged( const QString & text );
    virtual void addExtraDbPB_clicked();
    virtual void delExtraDbPB_clicked();
    virtual void browseDbPB_clicked();
    virtual void togExtraDbPB_clicked();
    virtual void actAllExtraDbPB_clicked();
    virtual void unacAllExtraDbPB_clicked();
    virtual void setStemLang(const QString& lang);

signals:
    void uiprefsDone();

protected slots:
    virtual void accept();
    virtual void reject();
private:
    void setFromPrefs();
    ViewAction *m_viewAction;

};


#endif /* _UIPREFS_W_H_INCLUDED_ */
