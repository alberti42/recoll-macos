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

#include <qlistwidget.h>

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
    connect(chgActPB, SIGNAL(clicked()), 
	    this, SLOT(editActions()));
    connect(actionsLV,SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	    this, SLOT(onItemDoubleClicked(QTableWidgetItem *)));
    fillLists();
    resize(QSize(640, 250).expandedTo(minimumSizeHint()));
}

void ViewAction::fillLists()
{
    actionsLV->clear();
    actionsLV->verticalHeader()->setDefaultSectionSize(20); 
    vector<pair<string, string> > defs;
    rclconfig->getMimeViewerDefs(defs);
    actionsLV->setRowCount(defs.size());
    int row = 0;
    for (vector<pair<string, string> >::const_iterator it = defs.begin();
	 it != defs.end(); it++) {
	actionsLV->setItem(row, 0, 
	   new QTableWidgetItem(QString::fromAscii(it->first.c_str())));
	actionsLV->setItem(row, 1, 
	   new QTableWidgetItem(QString::fromAscii(it->second.c_str())));
	row++;
    }
    QStringList labels(tr("Mime type"));
    labels.push_back(tr("Command"));
    actionsLV->setHorizontalHeaderLabels(labels);
}

void ViewAction::selectMT(const QString& mt)
{
    actionsLV->clearSelection();
    QList<QTableWidgetItem *>items = 
	actionsLV->findItems(mt, Qt::MatchFixedString|Qt::MatchCaseSensitive);
    for (QList<QTableWidgetItem *>::iterator it = items.begin();
	 it != items.end(); it++) {
	(*it)->setSelected(true);
	actionsLV->setCurrentItem(*it, QItemSelectionModel::Columns);
    }
}
void ViewAction::onItemDoubleClicked(QTableWidgetItem * item)
{
    actionsLV->clearSelection();
    item->setSelected(true);
    QTableWidgetItem *item0 = actionsLV->item(item->row(), 0);
    item0->setSelected(true);
    editActions();
}

void ViewAction::editActions()
{
    QString action0;
    list<string> mtypes;
    bool dowarnmultiple = true;
    for (int row = 0; row < actionsLV->rowCount(); row++) {
	QTableWidgetItem *item0 = actionsLV->item(row, 0);
	if (!item0->isSelected())
	    continue;
	mtypes.push_back((const char *)item0->text().local8Bit());
	QTableWidgetItem *item1 = actionsLV->item(row, 1);
	QString action = item1->text();
	if (action0.isEmpty()) {
	    action0 = action;
	} else {
	    if (action != action0 && dowarnmultiple) {
		switch (QMessageBox::warning(0, "Recoll",
					     tr("Changing actions with "
						"different current values"),
					     "Continue",
					     "Cancel",
					     0, 0, 1)) {
		case 0: dowarnmultiple = false;break;
		case 1: return;
		}
	    }
	}
    }
    if (action0.isEmpty())
	return;

    bool ok;
    QString newaction = QInputDialog::getText("Recoll", "Edit action:", 
					 QLineEdit::Normal,
					 action0, &ok, this);
    if (!ok || newaction.isEmpty() ) 
	return;

    string sact = (const char *)newaction.local8Bit();
    for (list<string>::const_iterator it = mtypes.begin(); 
	 it != mtypes.end(); it++) {
	rclconfig->setMimeViewerDef(*it, sact);
    }
    fillLists();
}
