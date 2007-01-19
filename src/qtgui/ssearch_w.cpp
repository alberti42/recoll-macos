#ifndef lint
static char rcsid[] = "@(#$Id: ssearch_w.cpp,v 1.18 2007-01-19 10:32:39 dockes Exp $ (C) 2006 J.F.Dockes";
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
#include <qapplication.h>
#include <qinputdialog.h>
#include <qvariant.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmessagebox.h>
#include <qevent.h>

#include "debuglog.h"
#include "guiutils.h"
#include "searchdata.h"
#include "ssearch_w.h"
#include "refcntr.h"
#include "textsplit.h"
#include "wasatorcl.h"

enum SSearchType {SST_ANY = 0, SST_ALL = 1, SST_FNM = 2, SST_LANG};

void SSearch::init()
{
    // See enum above and keep in order !
    searchTypCMB->insertItem(tr("Any term"));
    searchTypCMB->insertItem(tr("All terms"));
    searchTypCMB->insertItem(tr("File name"));
    searchTypCMB->insertItem(tr("Query language"));
    
    queryText->insertStringList(prefs.ssearchHistory);
    queryText->setEditText("");
    connect(queryText->lineEdit(), SIGNAL(returnPressed()),
	    this, SLOT(startSimpleSearch()));
    connect(queryText->lineEdit(), SIGNAL(textChanged(const QString&)),
	    this, SLOT(searchTextChanged(const QString&)));
    connect(clearqPB, SIGNAL(clicked()), 
	    queryText->lineEdit(), SLOT(clear()));
    connect(searchPB, SIGNAL(clicked()), this, SLOT(startSimpleSearch()));

    queryText->lineEdit()->installEventFilter(this);
    m_escape = false;
}

void SSearch::searchTextChanged( const QString & text )
{
    if (text.isEmpty()) {
	searchPB->setEnabled(false);
	clearqPB->setEnabled(false);
	emit clearSearch();
    } else {
	searchPB->setEnabled(true);
	clearqPB->setEnabled(true);
    }
}

void SSearch::startSimpleSearch()
{
    if (queryText->currentText().length() == 0)
	return;

    string u8 = (const char *)queryText->currentText().utf8();
    LOGDEB(("SSearch::startSimpleSearch: [%s]\n", u8.c_str()));

    SSearchType tp = (SSearchType)searchTypCMB->currentItem();
    Rcl::SearchData *sdata = 0;

    if (tp == SST_LANG) {
	string reason;
	sdata = wasaStringToRcl(u8, reason);
	if (sdata == 0) {
	    QMessageBox::warning(0, "Recoll", tr("Bad query string") +
				 QString::fromAscii(reason.c_str()));
	    return;
	}
    } else {
	sdata = new Rcl::SearchData(Rcl::SCLT_OR);
	if (sdata == 0) {
	    QMessageBox::warning(0, "Recoll", tr("Out of memory"));
	    return;
	}

	// Maybe add automatic phrase? 
	if (prefs.ssearchAutoPhrase && (tp == SST_ANY || tp == SST_ALL) &&
	    u8.find_first_of("\"") == string::npos && 
	    TextSplit::countWords(u8) > 1) {
	    sdata->addClause(new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, 
							   u8, 0));
	}

	switch (tp) {
	case SST_ANY:
	default: 
	    sdata->addClause(new Rcl::SearchDataClauseSimple(Rcl::SCLT_OR,u8));
	    break;
	case SST_ALL:
	   sdata->addClause(new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND,u8));
	    break;
	case SST_FNM:
	    sdata->addClause(new Rcl::SearchDataClauseFilename(u8));
	    break;
	}
    }

    // Search terms history

    // Need to remove any previous occurence of the search entry from
    // the listbox list, The qt listbox doesn't do lru correctly (if
    // already in the list the new entry would remain at it's place,
    // not jump at the top as it should
    LOGDEB3(("Querytext list count %d\n", queryText->count()));
    // Have to save current text, this will change while we clean up the list
    QString txt = queryText->currentText();
    bool changed;
    do {
	changed = false;
	for (int index = 0; index < queryText->count(); index++) {
	    LOGDEB3(("Querytext[%d] = [%s]\n", index,
		    (const char *)(queryText->text(index).utf8())));
	    if (queryText->text(index).length() == 0 || 
		QString::compare(queryText->text(index), txt) == 0) {
		LOGDEB3(("Querytext removing at %d [%s] [%s]\n", index,
			(const char *)(queryText->text(index).utf8()),
			(const char *)(txt.utf8())));
		queryText->removeItem(index);
		changed = true;
		break;
	    }
	}
    } while (changed);
    // The combobox is set for no insertion, insert here:
    queryText->insertItem(txt, 0);
    queryText->setCurrentItem(0);

    // Save the current state of the listbox list to the prefs (will
    // go to disk)
    prefs.ssearchHistory.clear();
    for (int index = 0; index < queryText->count(); index++) {
	prefs.ssearchHistory.push_back(queryText->text(index));
    }


    RefCntr<Rcl::SearchData> rsdata(sdata);
    emit startSearch(rsdata);
}

void SSearch::setAnyTermMode()
{
    searchTypCMB->setCurrentItem(SST_ANY);
}

// Complete last word in input by querying db for all possible terms.
void SSearch::completion()
{
    if (!rcldb)
	return;
    if (searchTypCMB->currentItem() == SST_FNM) {
	// Filename: no completion
	QApplication::beep();
	return;
    }
    // Extract last word in text
    string txt = (const char *)queryText->currentText().utf8();
    string::size_type cs = txt.find_last_of(" ");
    if (cs == string::npos)
	cs = 0;
    else
	cs++;
    if (txt.size() == 0 || cs == txt.size()) {
	QApplication::beep();
	return;
    }
    string s = txt.substr(cs) + "*";
    LOGDEB(("Completing: [%s]\n", s.c_str()));

    // Query database
    const int max = 100;
    list<Rcl::TermMatchEntry> strs;
    if (!rcldb->termMatch(Rcl::Db::ET_WILD, prefs.queryStemLang.ascii(),
			  s, strs, max) || strs.size() == 0) {
	QApplication::beep();
	return;
    }
    if (strs.size() == (unsigned int)max) {
	QMessageBox::warning(0, "Recoll", tr("Too many completions"));
	return;
    }

    // If list from db is single word, insert it, else ask user to select
    QString res;
    bool ok = false;
    if (strs.size() == 1) {
	res = QString::fromUtf8(strs.begin()->term.c_str());
	ok = true;
    } else {
	QStringList lst;
	for (list<Rcl::TermMatchEntry>::iterator it=strs.begin(); 
	     it != strs.end(); it++) {
	    lst.push_back(QString::fromUtf8(it->term.c_str()));
	}
	res = QInputDialog::getItem(tr("Completions"),
				    tr("Select an item:"), lst, 0, 
				    FALSE, &ok, this);
    }

    // Insert result
    if (ok) {
	txt.erase(cs);
	txt.append(res.utf8());
	queryText->setEditText(QString::fromUtf8(txt.c_str()));
    } else {
	return;
    }
}


bool SSearch::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)  {
	QKeyEvent *ke = (QKeyEvent *)event;
	LOGDEB1(("SSearch::eventFilter: keyPress escape %d key %d\n", 
		 m_escape, ke->key()));
	if (ke->key() == Qt::Key_Escape) {
	    LOGDEB(("Escape\n"));
	    m_escape = true;
	    return true;
	} else if (m_escape && ke->key() == Qt::Key_Space) {
	    LOGDEB(("Escape space\n"));
	    ke->accept();
	    completion();
	    m_escape = false;
	    return true;
	} else if (ke->key() == Qt::Key_Space) {
	    if (prefs.autoSearchOnWS)
		startSimpleSearch();
	}
	m_escape = false;
    }
    return QWidget::eventFilter(target, event);
}
