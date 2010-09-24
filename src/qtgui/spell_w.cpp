#ifndef lint
static char rcsid[] = "@(#$Id: spell_w.cpp,v 1.11 2007-02-19 16:28:05 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include "autoconfig.h"

#include <unistd.h>

#include <list>
#include <stdio.h>

#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qcombobox.h>
#if (QT_VERSION < 0x040000)
#include <qlistview.h>
#else
#include <QTableWidget>
#include <QHeaderView>
#endif

#include "debuglog.h"
#include "recoll.h"
#include "spell_w.h"
#include "guiutils.h"
#include "rcldb.h"
#include "rclhelp.h"

#ifdef RCL_USE_ASPELL
#include "rclaspell.h"
#endif

void SpellW::init()
{
    // Don't change the order, or fix the rest of the code...
    /*0*/expTypeCMB->insertItem(tr("Wildcards"));
    /*1*/expTypeCMB->insertItem(tr("Regexp"));
    /*2*/expTypeCMB->insertItem(tr("Stem expansion"));
#ifdef RCL_USE_ASPELL
    bool noaspell = false;
    rclconfig->getConfParam("noaspell", &noaspell);
    if (!noaspell)
	/*3*/expTypeCMB->insertItem(tr("Spelling/Phonetic"));
#endif

    int typ = prefs.termMatchType;
    if (typ < 0 || typ > expTypeCMB->count())
	typ = 0;
    expTypeCMB->setCurrentItem(typ);

    // Stemming language combobox
    stemLangCMB->clear();
    list<string> langs;
    if (!getStemLangs(langs)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("error retrieving stemming languages"));
    }
    for (list<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	stemLangCMB->
	    insertItem(QString::fromAscii(it->c_str(), it->length()));
    }
    stemLangCMB->setEnabled(expTypeCMB->currentItem()==2);

    (void)new HelpClient(this);
    HelpClient::installMap(this->name(), "RCL.SEARCH.TERMEXPLORER");

    // signals and slots connections
    connect(baseWordLE, SIGNAL(textChanged(const QString&)), 
	    this, SLOT(wordChanged(const QString&)));
    connect(baseWordLE, SIGNAL(returnPressed()), this, SLOT(doExpand()));
    connect(expandPB, SIGNAL(clicked()), this, SLOT(doExpand()));
    connect(dismissPB, SIGNAL(clicked()), this, SLOT(close()));
    connect(expTypeCMB, SIGNAL(activated(int)), this, SLOT(modeSet(int)));

#if (QT_VERSION < 0x040000)
    connect(suggsLV,
            SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),
            this, SLOT(textDoubleClicked()));
    // No initial sorting: user can choose to establish one
    suggsLV->setSorting(100, false);
#else
    QStringList labels(tr("Term"));
    labels.push_back(tr("Doc. / Tot."));
    suggsLV->setHorizontalHeaderLabels(labels);
    suggsLV->setShowGrid(0);
    suggsLV->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    connect(suggsLV,
	   SIGNAL(cellDoubleClicked(int, int)),
            this, SLOT(textDoubleClicked(int, int)));
#endif

    suggsLV->setColumnWidth(0, 200);
    suggsLV->setColumnWidth(1, 150);
}

#if (QT_VERSION < 0x040000)
// Subclass qlistviewitem for numeric sorting on column 1
class MyListViewItem : public QListViewItem
{
public:
    MyListViewItem(QListView *listView, const QString& s1, const QString& s2)
        : QListViewItem(listView, s1, s2)
    { }

    int compare(QListViewItem * i, int col, bool) const {
	if (col == 0)
	    return i->text(0).compare(text(0));
	if (col == 1)
	    return i->text(1).toInt() - text(1).toInt();
	// ??
	return 0;
    }
};
#else


#endif

/* Expand term according to current mode */
void SpellW::doExpand()
{
    // Can't clear qt4 table widget: resets column headers too
#if (QT_VERSION < 0x040000)
    suggsLV->clear();
#else
    suggsLV->setRowCount(0);
#endif
    if (baseWordLE->text().isEmpty()) 
	return;

    string reason;
    if (!maybeOpenDb(reason)) {
	LOGDEB(("SpellW::doExpand: db error: %s\n", reason.c_str()));
	return;
    }

    string expr = string((const char *)baseWordLE->text().utf8());
    list<string> suggs;

    prefs.termMatchType = expTypeCMB->currentItem();

    Rcl::Db::MatchType mt = Rcl::Db::ET_WILD;
    switch(expTypeCMB->currentItem()) {
    case 0: mt = Rcl::Db::ET_WILD; break;
    case 1:mt = Rcl::Db::ET_REGEXP; break;
    case 2:mt = Rcl::Db::ET_STEM; break;
    }

    Rcl::TermMatchResult res;
    switch (expTypeCMB->currentItem()) {
    case 0: 
    case 1:
    case 2: 
    {
	string l_stemlang = stemLangCMB->currentText().ascii();

	if (!rcldb->termMatch(mt, l_stemlang, expr, res, 200)) {
	    LOGERR(("SpellW::doExpand:rcldb::termMatch failed\n"));
	    return;
	}
        statsLBL->setText(tr("Index: %1 documents, average length %2 terms")
                          .arg(res.dbdoccount).arg(res.dbavgdoclen, 0, 'f', 1));
    }
        
	break;

#ifdef RCL_USE_ASPELL
    case 3: {
	LOGDEB(("SpellW::doExpand: aspelling\n"));
	if (!aspell) {
	    QMessageBox::warning(0, "Recoll",
				 tr("Aspell init failed. "
				    "Aspell not installed?"));
	    LOGDEB(("SpellW::doExpand: aspell init error\n"));
	    return;
	}
	list<string> suggs;
	if (!aspell->suggest(*rcldb, expr, suggs, reason)) {
	    QMessageBox::warning(0, "Recoll",
				 tr("Aspell expansion error. "));
	    LOGERR(("SpellW::doExpand:suggest failed: %s\n", reason.c_str()));
	}
	for (list<string>::const_iterator it = suggs.begin(); 
	     it != suggs.end(); it++) 
	    res.entries.push_back(Rcl::TermMatchEntry(*it));
    }
#endif
    }


    if (res.entries.empty()) {
#if (QT_VERSION < 0x040000)
	new MyListViewItem(suggsLV, tr("No expansion found"), "");
#else
        suggsLV->setItem(0, 0, new QTableWidgetItem(tr("No expansion found")));
#endif
    } else {
#if (QT_VERSION < 0x040000)
	for (list<Rcl::TermMatchEntry>::reverse_iterator it = 
                 res.entries.rbegin(); 
	     it != res.entries.rend(); it++) {
#else
        int row = 0;
	for (list<Rcl::TermMatchEntry>::iterator it = res.entries.begin(); 
	     it != res.entries.end(); it++) {
#endif
	    LOGDEB(("SpellW::expand: %6d [%s]\n", it->wcf, it->term.c_str()));
	    char num[20];
	    if (it->wcf)
		sprintf(num, "%d / %d",  it->docs, it->wcf);
	    else
		num[0] = 0;
#if (QT_VERSION < 0x040000)
	    new MyListViewItem(suggsLV, 
			      QString::fromUtf8(it->term.c_str()),
			      QString::fromAscii(num));
#else
            if (suggsLV->rowCount() <= row)
                suggsLV->setRowCount(row+1);
            suggsLV->setItem(row, 0, 
                    new QTableWidgetItem(QString::fromUtf8(it->term.c_str()))); 
            suggsLV->setItem(row++, 1, 
                             new QTableWidgetItem(QString::fromAscii(num)));
#endif
	}
#if (QT_VERSION >= 0x040000)
        suggsLV->setRowCount(row+1);
#endif
    }
}

void SpellW::wordChanged(const QString &text)
{
    if (text.isEmpty()) {
	expandPB->setEnabled(false);
#if (QT_VERSION < 0x040000)
        suggsLV->clear();
#else
        suggsLV->setRowCount(0);
#endif
    } else {
	expandPB->setEnabled(true);
    }
}

#if (QT_VERSION < 0x040000)
void SpellW::textDoubleClicked(int, int){}
void SpellW::textDoubleClicked()
#else
void SpellW::textDoubleClicked() {}
void SpellW::textDoubleClicked(int row, int)
#endif
{
#if (QT_VERSION < 0x040000)
    QListViewItemIterator it(suggsLV);
    while (it.current()) {
	QListViewItem *item = it.current();
	if (!item->isSelected()) {
	    ++it;
	    continue;
	}
	emit(wordSelect((const char *)item->text(0)));
	++it;
    }
#else
    QTableWidgetItem *item = suggsLV->item(row, 0);
    if (item)
        emit(wordSelect(item->text()));
#endif
}

void SpellW::modeSet(int mode)
{
    if (mode == 2)
	stemLangCMB->setEnabled(true);
    else
	stemLangCMB->setEnabled(false);
}
