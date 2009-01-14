#ifndef lint
static char rcsid[] = "@(#$Id: viewaction_w.cpp,v 1.5 2007-09-08 17:21:49 dockes Exp $ (C) 2006 J.F.Dockes";
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

#include <qpushbutton.h>
#include <qtimer.h>

#if (QT_VERSION < 0x040000)
#include <qlistview.h>
#define QLVEXACTMATCH Qt::ExactMatch
#else
#include <q3listview.h>
#define QListView Q3ListView
#define QListViewItem Q3ListViewItem
#define QListViewItemIterator Q3ListViewItemIterator
#define QLVEXACTMATCH Q3ListView::ExactMatch
#endif

#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qlayout.h>

#include "recoll.h"
#include "debuglog.h"
#include "guiutils.h"

#include "viewaction_w.h"

void ViewAction::init()
{
    connect(closePB, SIGNAL(clicked()), this, SLOT(close()));
    connect(chgActPB, SIGNAL(clicked()), this, SLOT(editAction()));
    connect(actionsLV,
#if (QT_VERSION < 0x040000)
	   SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),
#else
	   SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)),
#endif
	   this, SLOT(editAction()));
    fillLists();
    resize(QSize(640, 250).expandedTo(minimumSizeHint()) );
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

void ViewAction::selectMT(const QString& mt)
{
    QListViewItem *item =  actionsLV->findItem(mt, 0, QLVEXACTMATCH);
    if (item) {
	actionsLV->ensureItemVisible(item);
	actionsLV->setSelected(item, true);
	actionsLV->setSelectionAnchor(item);
    }
}

// To avoid modifying the listview state from the dbl click signal, as
// advised by the manual
void ViewAction::listDblClicked()
{
    QTimer::singleShot(0, this, SLOT(editAction()));
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
					     tr("Changing actions with different "
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
