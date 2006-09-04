#ifndef lint
static char rcsid[] = "@(#$Id: advsearch_w.cpp,v 1.1 2006-09-04 15:13:01 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include "advsearch_w.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qfiledialog.h>
#include <qmessagebox.h>

#include <list>
#include <string>

#ifndef NO_NAMESPACES
using std::list;
using std::string;
#endif /* NO_NAMESPACES */

#include "recoll.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "searchdata.h"

extern RclConfig *rclconfig;

void AdvSearch::init()
{
    // signals and slots connections
    connect( delFiltypPB, SIGNAL( clicked() ), this, SLOT( delFiltypPB_clicked() ) );
    connect( searchPB, SIGNAL( clicked() ), this, SLOT( searchPB_clicked() ) );
    connect( restrictFtCB, SIGNAL( toggled(bool) ), this, SLOT( restrictFtCB_toggled(bool) ) );
    connect( dismissPB, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( browsePB, SIGNAL( clicked() ), this, SLOT( browsePB_clicked() ) );
    connect( addFiltypPB, SIGNAL( clicked() ), this, SLOT( addFiltypPB_clicked() ) );
    connect( andWordsLE, SIGNAL( returnPressed() ), this, SLOT( searchPB_clicked() ) );
    connect( orWordsLE, SIGNAL( returnPressed() ), this, SLOT( searchPB_clicked() ) );
    connect( noWordsLE, SIGNAL( returnPressed() ), this, SLOT( searchPB_clicked() ) );
    connect( phraseLE, SIGNAL( returnPressed() ), this, SLOT( searchPB_clicked() ) );
    connect( subtreeLE, SIGNAL( returnPressed() ), this, SLOT( searchPB_clicked() ) );
    connect( delAFiltypPB, SIGNAL( clicked() ), this, SLOT( delAFiltypPB_clicked() ) );
    connect( addAFiltypPB, SIGNAL( clicked() ), this, SLOT( addAFiltypPB_clicked() ) );


    list<string> types = rclconfig->getAllMimeTypes();

    QStringList ql;
    for (list<string>::iterator it = types.begin(); it != types.end(); it++) {
	ql.append(it->c_str());
    }
    yesFiltypsLB->insertStringList(ql);

}


// Move selected file types from the searched to the ignored box
void AdvSearch::delFiltypPB_clicked()
{
    list<int> trl;
    QStringList moved;
    for (unsigned int i = 0; i < yesFiltypsLB->count();i++) {
	QListBoxItem *item = yesFiltypsLB->item(i);
	if (item && item->isSelected()) {
	    moved.push_back(item->text());
	    trl.push_front(i);
	}
    }
    if (!moved.empty()) {
	noFiltypsLB->insertStringList(moved);
	for (list<int>::iterator it = trl.begin();it != trl.end(); it++)
	    yesFiltypsLB->removeItem(*it);
    }
    yesFiltypsLB->sort();
    noFiltypsLB->sort();
}

void AdvSearch::delAFiltypPB_clicked()
{
    for (unsigned int i = 0; i < yesFiltypsLB->count();i++) {
	yesFiltypsLB->setSelected(i, true);
    }
    delFiltypPB_clicked();
}

// Move selected file types from the ignored to the searched box
void AdvSearch::addFiltypPB_clicked()
{
    list<int> trl;
    QStringList moved;
    for (unsigned int i = 0; i < noFiltypsLB->count(); i++) {
	QListBoxItem *item = noFiltypsLB->item(i);
	if (item && item->isSelected()) {
	    moved.push_back(item->text());
	    trl.push_front(i);
	}
    }
    if (!moved.empty()) {
	yesFiltypsLB->insertStringList(moved);
	for (list<int>::iterator it = trl.begin();it != trl.end(); it++)
	    noFiltypsLB->removeItem(*it);
    }
    yesFiltypsLB->sort();
    noFiltypsLB->sort();
}

void AdvSearch::addAFiltypPB_clicked()
{
    for (unsigned int i = 0; i < noFiltypsLB->count();i++) {
	noFiltypsLB->setSelected(i, true);
    }
    addFiltypPB_clicked();
}


// Activate file type selection
void AdvSearch::restrictFtCB_toggled(bool on)
{
    yesFiltypsLB->setEnabled(on);
    delFiltypPB->setEnabled(on);
    addFiltypPB->setEnabled(on);
    delAFiltypPB->setEnabled(on);
    addAFiltypPB->setEnabled(on);
    noFiltypsLB->setEnabled(on);
}

void AdvSearch::searchPB_clicked()
{
    Rcl::AdvSearchData mydata;

    mydata.allwords = string((const char*)(andWordsLE->text().utf8()));
    mydata.phrase  = string((const char*)(phraseLE->text().utf8()));
    mydata.orwords = string((const char*)(orWordsLE->text().utf8()));
    mydata.orwords1 = string((const char*)(orWords1LE->text().utf8()));
    mydata.nowords = string((const char*)(noWordsLE->text().utf8()));
    mydata.filename = string((const char*)(fileNameLE->text().utf8()));

    if (mydata.allwords.empty() && mydata.phrase.empty() && 
	mydata.orwords.empty() && mydata.orwords1.empty() && 
	mydata.filename.empty()) {
	if (mydata.nowords.empty())
	    return;
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot execute pure negative query. "
				"Please enter common terms in the 'any words' field")); 
	return;
    }

    if (restrictFtCB->isOn() && noFiltypsLB->count() > 0) {
	for (unsigned int i = 0; i < yesFiltypsLB->count(); i++) {
	    QCString ctext = yesFiltypsLB->item(i)->text().utf8();
	    mydata.filetypes.push_back(string((const char *)ctext));
	}
    }
    if (!subtreeLE->text().isEmpty()) {
	mydata.topdir = string((const char*)(subtreeLE->text().utf8()));
    }
    emit startSearch(mydata);
}


void AdvSearch::browsePB_clicked()
{
    QString dir = QFileDialog::getExistingDirectory();
    subtreeLE->setText(dir);
}

