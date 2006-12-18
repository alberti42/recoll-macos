#ifndef _VIEWACTION_W_H_INCLUDED_
#define _VIEWACTION_W_H_INCLUDED_
/* @(#$Id: viewaction_w.h,v 1.3 2006-12-18 16:45:52 dockes Exp $  (C) 2006 J.F.Dockes */
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
#include <qdialog.h>

#if (QT_VERSION < 0x040000)
#include "viewaction.h"
#else
#include "ui_viewaction.h"
#endif

class QDialog;
class QMouseEvent;

//MOC_SKIP_BEGIN
#if QT_VERSION < 0x040000
class DummyViewActionBase : public ViewActionBase
{
 public: DummyViewActionBase(QWidget* parent = 0) : ViewActionBase(parent) {}
};
#else
class DummyViewActionBase : public QDialog, public Ui::ViewActionBase
{
public:
    DummyViewActionBase(QWidget* parent)
	: QDialog(parent)
    {
	setupUi(this);
    }
};
#endif
//MOC_SKIP_END

class ViewAction : public DummyViewActionBase
{
    Q_OBJECT

public:
    ViewAction(QDialog* parent = 0) 
	: DummyViewActionBase(parent) 
    {
	init();
    }
    ~ViewAction() {}

public slots:
    virtual void editAction();
    virtual void listDblClicked();

private:
    virtual void init();
    virtual void fillLists();
};



#endif /* _VIEWACTION_W_H_INCLUDED_ */
