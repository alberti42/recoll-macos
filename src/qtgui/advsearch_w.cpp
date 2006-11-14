#ifndef lint
static char rcsid[] = "@(#$Id: advsearch_w.cpp,v 1.6 2006-11-14 13:55:43 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <qcombobox.h>
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
#include "guiutils.h"

extern RclConfig *rclconfig;

void AdvSearch::init()
{
    // signals and slots connections
    connect(delFiltypPB, SIGNAL(clicked()), this, SLOT(delFiltypPB_clicked()));
    connect(searchPB, SIGNAL(clicked()), this, SLOT(searchPB_clicked()));
    connect(restrictFtCB, SIGNAL(toggled(bool)), 
	    this, SLOT(restrictFtCB_toggled(bool)));
    connect(dismissPB, SIGNAL(clicked()), this, SLOT(close()));
    connect(browsePB, SIGNAL(clicked()), this, SLOT(browsePB_clicked()));
    connect(addFiltypPB, SIGNAL(clicked()), this, SLOT(addFiltypPB_clicked()));
    connect(andWordsLE, SIGNAL(returnPressed()), 
	    this, SLOT(searchPB_clicked()));
    connect(orWordsLE, SIGNAL(returnPressed()), 
	    this, SLOT(searchPB_clicked()));
    connect(noWordsLE, SIGNAL(returnPressed()), 
	    this, SLOT(searchPB_clicked()));
    connect(phraseLE, SIGNAL(returnPressed()), this, SLOT(searchPB_clicked()));
    connect(subtreeCMB->lineEdit(), SIGNAL(returnPressed()), 
	    this, SLOT(searchPB_clicked()));
    connect(delAFiltypPB, SIGNAL(clicked()), 
	    this, SLOT(delAFiltypPB_clicked()));
    connect(addAFiltypPB, SIGNAL(clicked()), 
	    this, SLOT(addAFiltypPB_clicked()));
    connect(saveFileTypesPB, SIGNAL(clicked()), 
	    this, SLOT(saveFileTypes()));
    connect(addClausePB, SIGNAL(clicked()), this, SLOT(addClause()));

    // Initialize lists of accepted and ignored mime types from config
    // and settings
    list<string> types = rclconfig->getAllMimeTypes();
    noFiltypsLB->insertStringList(prefs.asearchIgnFilTyps); 

    QStringList ql;
    for (list<string>::iterator it = types.begin(); it != types.end(); it++) {
	if (prefs.asearchIgnFilTyps.findIndex(it->c_str())<0)
	    ql.append(it->c_str());
    }
    yesFiltypsLB->insertStringList(ql);

    subtreeCMB->insertStringList(prefs.asearchSubdirHist);
    subtreeCMB->setEditText("");
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

// Save current list of ignored file types to prefs
void AdvSearch::saveFileTypes()
{
    prefs.asearchIgnFilTyps.clear();
    for (unsigned int i = 0; i < noFiltypsLB->count();i++) {
	QListBoxItem *item = noFiltypsLB->item(i);
	prefs.asearchIgnFilTyps.append(item->text());
    }
    rwSettings(true);
}

void AdvSearch::addClause()
{
    SearchClauseW *w = new SearchClauseW(this);
    m_clauseWins.push_back(w);
    connect(w->wordsLE, SIGNAL(returnPressed()),
	    this, SLOT(searchPB_clicked()));
    clauseVBox->insertWidget(-1, w);
    w->show();
    // Have to adjust the size else we lose the bottom buttons! Why?
    QSize sz = AdvSearchBaseLayout->sizeHint();
    resize(QSize(sz.width()+20, sz.height()+40));
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

using namespace Rcl;
void AdvSearch::searchPB_clicked()
{
    RefCntr<SearchData> sdata(new SearchData(SCLT_AND));
    bool hasnotnot = false;
    bool hasnot = false;
    if (!andWordsLE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseSimple(SCLT_AND,
				    (const char *)andWordsLE->text().utf8()));
	hasnotnot = true;
    }
    if (!orWordsLE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseSimple(SCLT_OR,
				    (const char *)orWordsLE->text().utf8()));
	hasnotnot = true;
    }
    if (!orWords1LE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseSimple(SCLT_OR,
				    (const char *)orWords1LE->text().utf8()));
	hasnotnot = true;
    }
    if (!noWordsLE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseSimple(SCLT_EXCL,
				    (const char *)noWordsLE->text().utf8()));
	hasnot = true;
    }
    if (!fileNameLE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseFilename(
				     (const char *)fileNameLE->text().utf8()));
	hasnotnot = true;
    }
    if (!phraseLE->text().isEmpty()) {
	sdata->addClause(new SearchDataClauseDist(SCLT_PHRASE,
				  (const char *)phraseLE->text().utf8(), 0));
	hasnotnot = true;
    }
    for (list<SearchClauseW*>::iterator it = m_clauseWins.begin();
	 it != m_clauseWins.end(); it++) {
	SearchDataClause *cl;
	if ((cl = (*it)->getClause())) {
	    switch (cl->m_tp) {
	    case SCLT_EXCL: hasnot = true; break;
	    default: hasnotnot = true; break;
	    }
	    sdata->addClause(cl);
	}
    }
    if (!hasnotnot) {
	if (!hasnot)
	    return;
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot execute pure negative query. "
				"Please enter common terms in the 'any words' field")); 
	return;
    }
    if (restrictFtCB->isOn() && noFiltypsLB->count() > 0) {
	for (unsigned int i = 0; i < yesFiltypsLB->count(); i++) {
	    QCString ctext = yesFiltypsLB->item(i)->text().utf8();
	    sdata->m_filetypes.push_back(string((const char *)ctext));
	}
    }

    if (!subtreeCMB->currentText().isEmpty()) {
	sdata->m_topdir = 
	    string((const char*)(subtreeCMB->currentText().utf8()));
	// The listbox is set for no insertion, do it. Have to check
	// for dups as the internal feature seems to only work for
	// user-inserted strings
	if (!subtreeCMB->listBox()->findItem(subtreeCMB->currentText(),
					     Qt::CaseSensitive|Qt::ExactMatch))
	    subtreeCMB->insertItem(subtreeCMB->currentText(), 0);
	// And keep it sorted
	subtreeCMB->listBox()->sort();
	prefs.asearchSubdirHist.clear();
	for (int index = 0; index < subtreeCMB->count(); index++)
	    prefs.asearchSubdirHist.push_back(subtreeCMB->text(index));
    }
    emit startSearch(sdata);
}


void AdvSearch::browsePB_clicked()
{
    QString dir = QFileDialog::getExistingDirectory();
    subtreeCMB->setEditText(dir);
}
