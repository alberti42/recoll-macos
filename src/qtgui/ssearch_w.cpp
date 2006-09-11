#ifndef lint
static char rcsid[] = "@(#$Id: ssearch_w.cpp,v 1.2 2006-09-11 07:10:25 dockes Exp $ (C) 2006 J.F.Dockes";
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

#include "debuglog.h"
#include "guiutils.h"
#include "searchdata.h"
#include "ssearch_w.h"

void SSearch::init()
{
    searchTypCMB->insertItem(tr("Any term"));
    searchTypCMB->insertItem(tr("All terms"));
    searchTypCMB->insertItem(tr("File name"));
    queryText->insertStringList(prefs.ssearchHistory);
    queryText->setEditText("");
    connect(queryText->lineEdit(), SIGNAL(returnPressed()),
	    this, SLOT(startSimpleSearch()));
    connect(queryText->lineEdit(), SIGNAL(textChanged(const QString&)),
	    this, SLOT(searchTextChanged(const QString&)));
    connect(clearqPB, SIGNAL(clicked()), 
	    queryText->lineEdit(), SLOT(clear()));
    connect(searchPB, SIGNAL(clicked()), this, SLOT(startSimpleSearch()));
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
	string u8 =  (const char *)queryText->currentText().utf8();
	if (prefs.autoSearchOnWS && !u8.empty() && u8[u8.length()-1] == ' ')
	    startSimpleSearch();
    }
}

void SSearch::startSimpleSearch()
{
    LOGDEB(("SSearch::startSimpleSearch\n"));

    Rcl::AdvSearchData sdata;
    QCString u8 =  queryText->currentText().utf8();
    switch (searchTypCMB->currentItem()) {
    case 0:
    default:
	sdata.orwords = u8;
	break;
    case 1:
	sdata.allwords = u8;
	break;
    case 2:
	sdata.filename = u8;
	break;
    }

    // Need to remove any previous occurence of the search entry from
    // the listbox list, The qt listbox doesn't do lru correctly (if
    // already in the list the new entry would remain at it's place,
    // not jump at the top as it should
    bool changed;
    do {
	changed = false;
	for (int index = 1; index < queryText->count(); index++) {
	    if (queryText->text(index) == queryText->currentText()) {
		queryText->removeItem(index);
		changed = true;
		break;
	    }
	}
    } while (changed);

    // Save the current state of the listbox list to file
    prefs.ssearchHistory.clear();
    for (int index = 0; index < queryText->count(); index++)
	prefs.ssearchHistory.push_back(queryText->text(index));

    emit startSearch(sdata);
}

void SSearch::setAnyTermMode()
{
    searchTypCMB->setCurrentItem(0);
}

// Complete last word in input by querying db for all possible terms.
void SSearch::completion()
{
    if (!rcldb)
	return;
    if (searchTypCMB->currentItem() == 2) {
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
    string s = txt.substr(cs);
    LOGDEB(("Completing: [%s]\n", s.c_str()));

    // Query database
    const int max = 100;
    list<string> strs = rcldb->completions(s, prefs.queryStemLang.ascii(),max);
    if (strs.size() == 0 || strs.size() == (unsigned int)max) {
	QApplication::beep();
	return;
    }

    // If list from db is single word, insert it, else ask user to select
    QString res;
    bool ok = false;
    if (strs.size() == 1) {
	res = QString::fromUtf8(strs.begin()->c_str());
	ok = true;
    } else {
	QStringList lst;
	for (list<string>::iterator it=strs.begin(); it != strs.end(); it++) 
	    lst.push_back(QString::fromUtf8(it->c_str()));
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

// Handle CTRL-TAB to mean completion
bool SSearch::event( QEvent *evt ) 
{
    if ( evt->type() == QEvent::KeyPress ) {
	QKeyEvent *ke = (QKeyEvent *)evt;
	if ( ke->key() == Key_Tab  && (ke->state() & Qt::ControlButton)) {
	    // special tab handling here
	    completion();
	    ke->accept();
	    return TRUE;
	}
    }
    return QWidget::event( evt );
}

