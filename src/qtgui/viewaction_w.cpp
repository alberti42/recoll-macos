#ifndef lint
static char rcsid[] = "@(#$Id: viewaction_w.cpp,v 1.2 2006-12-18 12:05:29 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
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
#include <vector>
#include <utility>
#include <string>

using namespace std;

#include <qcombobox.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qinputdialog.h>

#include "recoll.h"
#include "debuglog.h"
#include "guiutils.h"

#include "viewaction_w.h"

void ViewAction::init()
{
    connect(closePB, SIGNAL(clicked()), this, SLOT(close()));
    connect(chgActPB, SIGNAL(clicked()), this, SLOT(editAction()));
   connect(actionsLV,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),
	   this, SLOT(editAction()));
    fillLists();
    actionsLV->setColumnWidth(0, 150);
    actionsLV->setColumnWidth(1, 150);
    resize(550,350);
}
void ViewAction::fillLists()
{
    actionsLV->clear();
    vector<pair<string, string> > defs;
    rclconfig->getMimeViewerDefs(defs);
    for (vector<pair<string, string> >::const_iterator it = defs.begin();
	 it != defs.end(); it++) {
	new QListViewItem(actionsLV, 
			  QString::fromAscii(it->first.c_str()),
			  QString::fromAscii(it->second.c_str()));
    }

}

void ViewAction::editAction()
{
    QString action0;
    list<string> mtypes;
    bool dowarnmultiple = true;

    QListViewItemIterator it(actionsLV);
    while (it.current()) {
	QListViewItem *item = it.current();
	if (!item->isSelected()) {
	    ++it;
	    continue;
	}
	mtypes.push_back((const char *)item->text(0).utf8());
	QString action = (const char *)item->text(1).utf8();
	if (action0.isEmpty()) {
	    action0 = action;
	} else {
	    if (action != action0 && dowarnmultiple) {
		switch (QMessageBox::warning(0, "Recoll",
					     tr("Changing actions with different"
						"current values"),
					     "Continue",
					     "Cancel",
					     0, 0, 1)) {
		case 0: dowarnmultiple = false;break;
		case 1: return;
		}
	    }
	}
	++it;
    }
    if (action0.isEmpty())
	return;

    bool ok;
    QString text = QInputDialog::getText(
					 "Recoll", "Edit action:", 
					 QLineEdit::Normal,
					 action0, &ok, this);
    if (!ok || text.isEmpty() ) 
	return;

    string sact = (const char *)text.utf8();
    for (list<string>::const_iterator it = mtypes.begin(); 
	 it != mtypes.end(); it++) {
	rclconfig->setMimeViewerDef(*it, sact);
    }
    fillLists();
}
