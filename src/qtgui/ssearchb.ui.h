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
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include <qapplication.h>
#include <qinputdialog.h>

#include "debuglog.h"
#include "guiutils.h"
#include "searchdata.h"

void SSearchBase::init()
{
    searchTypCMB->insertItem(tr("Any term"));
    searchTypCMB->insertItem(tr("All terms"));
    searchTypCMB->insertItem(tr("File name"));
}

void SSearchBase::searchTextChanged( const QString & text )
{
    if (text.isEmpty()) {
	searchPB->setEnabled(false);
	clearqPB->setEnabled(false);
	emit clearSearch();
    } else {
	searchPB->setEnabled(true);
	clearqPB->setEnabled(true);
	string u8 =  (const char *)queryText->text().utf8();
	if (prefs.autoSearchOnWS && !u8.empty() && u8[u8.length()-1] == ' ')
	    startSimpleSearch();
    }
}

void SSearchBase::startSimpleSearch()
{
    LOGDEB(("SSearchBase::startSimpleSearch\n"));

    Rcl::AdvSearchData sdata;
    QCString u8 =  queryText->text().utf8();
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

    emit startSearch(sdata);
}

// Complete last word in input by querying db for all possible terms.
void SSearchBase::completion()
{
    if (!rcldb)
	return;
    if (searchTypCMB->currentItem() == 2) {
	// Filename: no completion
	QApplication::beep();
	return;
    }
    // Extract last word in text
    string txt = (const char *)queryText->text().utf8();
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
	queryText->setText(QString::fromUtf8(txt.c_str()));
    } else {
	return;
    }
}

// Handle CTRL-TAB to mean completion
bool SSearchBase::event( QEvent *evt ) 
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

