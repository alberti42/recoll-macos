/* Copyright (C) 2005 J.F.Dockes
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

#include <algorithm>

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
#include <QTableWidget>
#include <QHeaderView>
#include <QClipboard>
#include <QKeyEvent>

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
    /*0*/expTypeCMB->addItem(tr("Wildcards"));
    /*1*/expTypeCMB->addItem(tr("Regexp"));
    /*2*/expTypeCMB->addItem(tr("Stem expansion"));
#ifdef RCL_USE_ASPELL
    bool noaspell = false;
    theconfig->getConfParam("noaspell", &noaspell);
    if (!noaspell)
	/*3*/expTypeCMB->addItem(tr("Spelling/Phonetic"));
#endif

    int typ = prefs.termMatchType;
    if (typ < 0 || typ > expTypeCMB->count())
	typ = 0;
    expTypeCMB->setCurrentIndex(typ);

    // Stemming language combobox
    stemLangCMB->clear();
    vector<string> langs;
    if (!getStemLangs(langs)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("error retrieving stemming languages"));
    }
    for (vector<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	stemLangCMB->
	    addItem(QString::fromAscii(it->c_str(), it->length()));
    }
    stemLangCMB->setEnabled(expTypeCMB->currentIndex()==2);

    (void)new HelpClient(this);
    HelpClient::installMap((const char *)this->objectName().toUtf8(), 
			   "RCL.SEARCH.TERMEXPLORER");

    // signals and slots connections
    connect(baseWordLE, SIGNAL(textChanged(const QString&)), 
	    this, SLOT(wordChanged(const QString&)));
    connect(baseWordLE, SIGNAL(returnPressed()), this, SLOT(doExpand()));
    connect(expandPB, SIGNAL(clicked()), this, SLOT(doExpand()));
    connect(dismissPB, SIGNAL(clicked()), this, SLOT(close()));
    connect(expTypeCMB, SIGNAL(activated(int)), this, SLOT(modeSet(int)));

    QStringList labels(tr("Term"));
    labels.push_back(tr("Doc. / Tot."));
    resTW->setHorizontalHeaderLabels(labels);
    resTW->setShowGrid(0);
    resTW->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    resTW->verticalHeader()->setDefaultSectionSize(20); 
    connect(resTW,
	   SIGNAL(cellDoubleClicked(int, int)),
            this, SLOT(textDoubleClicked(int, int)));

    resTW->setColumnWidth(0, 200);
    resTW->setColumnWidth(1, 150);
    resTW->installEventFilter(this);
}

/* Expand term according to current mode */
void SpellW::doExpand()
{
    // Can't clear qt4 table widget: resets column headers too
    resTW->setRowCount(0);
    if (baseWordLE->text().isEmpty()) 
	return;

    string reason;
    if (!maybeOpenDb(reason)) {
	LOGDEB(("SpellW::doExpand: db error: %s\n", reason.c_str()));
	return;
    }

    string expr = string((const char *)baseWordLE->text().toUtf8());
    list<string> suggs;

    prefs.termMatchType = expTypeCMB->currentIndex();

    Rcl::Db::MatchType mt = Rcl::Db::ET_WILD;
    switch(expTypeCMB->currentIndex()) {
    case 0: mt = Rcl::Db::ET_WILD; break;
    case 1:mt = Rcl::Db::ET_REGEXP; break;
    case 2:mt = Rcl::Db::ET_STEM; break;
    }

    Rcl::TermMatchResult res;
    switch (expTypeCMB->currentIndex()) {
    case 0: 
    case 1:
    case 2: 
    {
	string l_stemlang = (const char*)stemLangCMB->currentText().toAscii();

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
#ifdef TESTING_XAPIAN_SPELL
	string rclsugg = rcldb->getSpellingSuggestion(expr);
	if (!rclsugg.empty()) {
	    res.entries.push_back(Rcl::TermMatchEntry("Xapian spelling:"));
	    res.entries.push_back(Rcl::TermMatchEntry(rclsugg));
	}
#endif // TESTING_XAPIAN_SPELL
    }
#endif
    }


    if (res.entries.empty()) {
        resTW->setItem(0, 0, new QTableWidgetItem(tr("No expansion found")));
    } else {
        int row = 0;
	for (vector<Rcl::TermMatchEntry>::iterator it = res.entries.begin(); 
	     it != res.entries.end(); it++) {
	    LOGDEB(("SpellW::expand: %6d [%s]\n", it->wcf, it->term.c_str()));
	    char num[30];
	    if (it->wcf)
		sprintf(num, "%d / %d",  it->docs, it->wcf);
	    else
		num[0] = 0;
            if (resTW->rowCount() <= row)
                resTW->setRowCount(row+1);
            resTW->setItem(row, 0, 
                    new QTableWidgetItem(QString::fromUtf8(it->term.c_str()))); 
            resTW->setItem(row++, 1, 
                             new QTableWidgetItem(QString::fromAscii(num)));
	}
        resTW->setRowCount(row+1);
    }
}

void SpellW::wordChanged(const QString &text)
{
    if (text.isEmpty()) {
	expandPB->setEnabled(false);
        resTW->setRowCount(0);
    } else {
	expandPB->setEnabled(true);
    }
}

void SpellW::textDoubleClicked() {}
void SpellW::textDoubleClicked(int row, int)
{
    QTableWidgetItem *item = resTW->item(row, 0);
    if (item)
        emit(wordSelect(item->text()));
}

void SpellW::modeSet(int mode)
{
    if (mode == 2)
	stemLangCMB->setEnabled(true);
    else
	stemLangCMB->setEnabled(false);
}

void SpellW::copy()
{
  QItemSelectionModel * selection = resTW->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();

  if(indexes.size() < 1)
    return;

  // QModelIndex::operator < sorts first by row, then by column. 
  // this is what we need
  std::sort(indexes.begin(), indexes.end());

  // You need a pair of indexes to find the row changes
  QModelIndex previous = indexes.first();
  indexes.removeFirst();
  QString selected_text;
  QModelIndex current;
  Q_FOREACH(current, indexes)
  {
    QVariant data = resTW->model()->data(previous);
    QString text = data.toString();
    // At this point `text` contains the text in one cell
    selected_text.append(text);
    // If you are at the start of the row the row number of the previous index
    // isn't the same.  Text is followed by a row separator, which is a newline.
    if (current.row() != previous.row())
    {
      selected_text.append(QLatin1Char('\n'));
    }
    // Otherwise it's the same row, so append a column separator, which is a tab.
    else
    {
      selected_text.append(QLatin1Char('\t'));
    }
    previous = current;
  }

  // add last element
  selected_text.append(resTW->model()->data(current).toString());
  selected_text.append(QLatin1Char('\n'));
  qApp->clipboard()->setText(selected_text, QClipboard::Selection);
  qApp->clipboard()->setText(selected_text, QClipboard::Clipboard);
}


bool SpellW::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::KeyPress ||
	(target != resTW && target != resTW->viewport())) 
	return false;

    QKeyEvent *keyEvent = (QKeyEvent *)event;
    if(keyEvent->matches(QKeySequence::Copy) )
    {
	copy();
	return true;
    }
    return false;
}
