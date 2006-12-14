#ifndef lint
static char rcsid[] = "@(#$Id: uiprefs_w.cpp,v 1.13 2006-12-14 13:53:43 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <sys/stat.h>

#include <string>
#include <algorithm>
#include <list>

#include <qfontdialog.h>
#include <qspinbox.h>
#include <qmessagebox.h>
#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#if QT_VERSION < 0x040000
#include <qlistbox.h>
#include <qfiledialog.h>
#else
#include <q3listbox.h>
#include <q3filedialog.h>
#define QListBox Q3ListBox
#define QListBoxItem Q3ListBoxItem
#define QFileDialog  Q3FileDialog
#endif
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtextedit.h>

#include "recoll.h"
#include "guiutils.h"
#include "rcldb.h"
#include "pathut.h"
#include "uiprefs_w.h"
#include "viewaction_w.h"

void UIPrefsDialog::init()
{
    m_viewAction = 0;

    connect(viewActionPB, SIGNAL(clicked()), this, SLOT(showViewAction()));
    connect(reslistFontPB, SIGNAL(clicked()), this, SLOT(showFontDialog()));
    connect(helpBrowserPB, SIGNAL(clicked()), this, SLOT(showBrowserDialog()));
    connect(resetFontPB, SIGNAL(clicked()), this, SLOT(resetReslistFont()));
    connect(extraDbLE,SIGNAL(textChanged(const QString&)), this, 
	    SLOT(extraDbTextChanged(const QString&)));
    connect(addAADbPB, SIGNAL(clicked()), this, SLOT(addAADbPB_clicked()));
    connect(addADbPB, SIGNAL(clicked()), this, SLOT(addADbPB_clicked()));
    connect(delADbPB, SIGNAL(clicked()), this, SLOT(delADbPB_clicked()));
    connect(delAADbPB, SIGNAL(clicked()), this, SLOT(delAADbPB_clicked()));
    connect(addExtraDbPB, SIGNAL(clicked()), this, SLOT(addExtraDbPB_clicked()));
    connect(browseDbPB, SIGNAL(clicked()), this, SLOT(browseDbPB_clicked()));
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(buildAbsCB, SIGNAL(toggled(bool)), 
	    replAbsCB, SLOT(setEnabled(bool)));
    setFromPrefs();
}

// Update dialog state from stored prefs
void UIPrefsDialog::setFromPrefs()
{
    // Entries per result page spinbox
    pageLenSB->setValue(prefs.respagesize);
    // Show icons checkbox
    useIconsCB->setChecked(prefs.showicons);
    autoSearchCB->setChecked(prefs.autoSearchOnWS);
    syntlenSB->setValue(prefs.syntAbsLen);
    syntctxSB->setValue(prefs.syntAbsCtx);

    initStartAdvCB->setChecked(prefs.startWithAdvSearchOpen);
    initStartSortCB->setChecked(prefs.startWithSortToolOpen);

    // Result list font family and size
    reslistFontFamily = prefs.reslistfontfamily;
    reslistFontSize = prefs.reslistfontsize;
    QString s;
    if (prefs.reslistfontfamily.length() == 0) {
	reslistFontPB->setText(this->font().family() + "-" +
			       s.setNum(this->font().pointSize()));
    } else {
	reslistFontPB->setText(reslistFontFamily + "-" +
			       s.setNum(reslistFontSize));
    }
    rlfTE->setText(prefs.reslistformat);
    helpBrowserLE->setText(prefs.htmlBrowser);

    // Stemming language combobox
    stemLangCMB->clear();
    stemLangCMB->insertItem(tr("(no stemming)"));
    list<string> langs;
    if (!getStemLangs(langs)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("error retrieving stemming languages"));
    }
    int i = 0, cur = -1;
    for (list<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	stemLangCMB->
	    insertItem(QString::fromAscii(it->c_str(), it->length()));
	i++;
	if (cur == -1) {
	    if (!strcmp(prefs.queryStemLang.ascii(), it->c_str()))
		cur = i;
	}
    }
    if (cur < 0)
	cur = 0;
    stemLangCMB->setCurrentItem(cur);

    autoPhraseCB->setChecked(prefs.ssearchAutoPhrase);

    buildAbsCB->setChecked(prefs.queryBuildAbstract);
    replAbsCB->setEnabled(prefs.queryBuildAbstract);
    replAbsCB->setChecked(prefs.queryReplaceAbstract);

    // Initialize the extra indexes listboxes
    QStringList ql;
    for (list<string>::iterator it = prefs.allExtraDbs.begin(); 
	 it != prefs.allExtraDbs.end(); it++) {
	ql.append(QString::fromLocal8Bit(it->c_str()));
    }
    allDbsLB->clear();
    allDbsLB->insertStringList(ql);
    ql.clear();
    for (list<string>::iterator it = prefs.activeExtraDbs.begin(); 
	 it != prefs.activeExtraDbs.end(); it++) {
	ql.append(QString::fromLocal8Bit(it->c_str()));
    }
    actDbsLB->clear();
    actDbsLB->insertStringList(ql);
    ql.clear();
}

void UIPrefsDialog::accept()
{
    prefs.showicons = useIconsCB->isChecked();
    prefs.autoSearchOnWS = autoSearchCB->isChecked();
    prefs.respagesize = pageLenSB->value();

    prefs.reslistfontfamily = reslistFontFamily;
    prefs.reslistfontsize = reslistFontSize;
    prefs.reslistformat =  rlfTE->text();

    prefs.htmlBrowser =  helpBrowserLE->text();

    if (stemLangCMB->currentItem() == 0) {
	prefs.queryStemLang = "";
    } else {
	prefs.queryStemLang = stemLangCMB->currentText();
    }
    prefs.ssearchAutoPhrase = autoPhraseCB->isChecked();
    prefs.queryBuildAbstract = buildAbsCB->isChecked();
    prefs.queryReplaceAbstract = buildAbsCB->isChecked() && 
	replAbsCB->isChecked();

    prefs.startWithAdvSearchOpen = initStartAdvCB->isChecked();
    prefs.startWithSortToolOpen = initStartSortCB->isChecked();

    prefs.syntAbsLen = syntlenSB->value();
    prefs.syntAbsCtx = syntctxSB->value();

    prefs.activeExtraDbs.clear();
    for (unsigned int i = 0; i < actDbsLB->count(); i++) {
	QListBoxItem *item = actDbsLB->item(i);
	if (item)
	    prefs.activeExtraDbs.push_back((const char *)item->text().local8Bit());
    }
    prefs.allExtraDbs.clear();
    for (unsigned int i = 0; i < allDbsLB->count(); i++) {
	QListBoxItem *item = allDbsLB->item(i);
	if (item)
	    prefs.allExtraDbs.push_back((const char *)item->text().local8Bit());
    }


    rwSettings(true);
    string reason;
    maybeOpenDb(reason, true);
    emit uiprefsDone();
    QDialog::accept();
}
void UIPrefsDialog::reject()
{
    setFromPrefs();
    QDialog::reject();
}

void UIPrefsDialog::showFontDialog()
{
    bool ok;
    QFont font;
    if (prefs.reslistfontfamily.length()) {
	font.setFamily(prefs.reslistfontfamily);
	font.setPointSize(prefs.reslistfontsize);
    }

    font = QFontDialog::getFont(&ok, font, this);
    if (ok) {
	// Check if the default font was set, in which case we
	// erase the preference
	if (font.family().compare(this->font().family()) || 
	    font.pointSize() != this->font().pointSize()) {
	    reslistFontFamily = font.family();
	    reslistFontSize = font.pointSize();
	    QString s;
	    reslistFontPB->setText(reslistFontFamily + "-" +
			       s.setNum(reslistFontSize));
	} else {
	    reslistFontFamily = "";
	    reslistFontSize = 0;
	}
    }
}


void UIPrefsDialog::resetReslistFont()
{
    reslistFontFamily = "";
    reslistFontSize = 0;
    reslistFontPB->setText(this->font().family() + "-" +
			   QString().setNum(this->font().pointSize()));
}

void UIPrefsDialog::showBrowserDialog()
{
    QString s = QFileDialog::getOpenFileName("/usr",
					     "",
					     this,
					     "open file dialog",
					     "Choose a file");
    if (!s.isEmpty()) 
	helpBrowserLE->setText(s);
}

void UIPrefsDialog::showViewAction()
{
    if (m_viewAction== 0) {
	m_viewAction = new ViewAction(0);
	m_viewAction->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	m_viewAction->close();
	m_viewAction->show();
    }
}

////////////////////////////////////////////
// External / extra search indexes setup
// TBD: a way to remove entry from 'all' list (del button?)

void UIPrefsDialog::extraDbTextChanged(const QString &text)
{
    if (text.isEmpty()) {
	addExtraDbPB->setEnabled(false);
    } else {
	addExtraDbPB->setEnabled(true);
    }
}

/** 
 * Add the selected extra dbs to the active list
 */
void UIPrefsDialog::addADbPB_clicked()
{
    for (unsigned int i = 0; i < allDbsLB->count();i++) {
	QListBoxItem *item = allDbsLB->item(i);
	if (item && item->isSelected()) {
	    allDbsLB->setSelected(i, false);
	    if (!actDbsLB->findItem(item->text(), 
#if QT_VERSION < 0x040000
			    Qt::CaseSensitive|Qt::ExactMatch
#else
			    Q3ListBox::CaseSensitive|Q3ListBox::ExactMatch
#endif
				    )) {
		actDbsLB->insertItem(item->text());
	    }
	}
    }
    actDbsLB->sort();
}

/**
 * Make all extra dbs active
 */
void UIPrefsDialog::addAADbPB_clicked()
{
    for (unsigned int i = 0; i < allDbsLB->count();i++) {
	allDbsLB->setSelected(i, true);
    }
    addADbPB_clicked();
}

/**
 * Remove the selected entries from the list of active extra search dbs
 */
void UIPrefsDialog::delADbPB_clicked()
{
    list<int> rmi;
    for (unsigned int i = 0; i < actDbsLB->count(); i++) {
	QListBoxItem *item = actDbsLB->item(i);
	if (item && item->isSelected()) {
	    rmi.push_front(i);
	}
    }
    for (list<int>::iterator ii = rmi.begin(); ii != rmi.end(); ii++) {
	actDbsLB->removeItem(*ii);
    }
}

/**
 * Remove all extra search indexes from the active list
 */
void UIPrefsDialog::delAADbPB_clicked()
{
    for (unsigned int i = 0; i < actDbsLB->count(); i++) {
	actDbsLB->setSelected(i, true);
    }
    delADbPB_clicked();
}

/** 
 * Add the current content of the extra db line editor to the list of all
 * extra dbs. We do a textual comparison to check for duplicates, except for
 * the main db for which we check inode numbers. 
 */
void UIPrefsDialog::addExtraDbPB_clicked()
{
    string dbdir = (const char *)extraDbLE->text().local8Bit();
    path_catslash(dbdir);
    if (!Rcl::Db::testDbDir(dbdir)) {
	QMessageBox::warning(0, "Recoll", 
        tr("The selected directory does not appear to be a Xapian index"));
	return;
    }
    struct stat st1, st2;
    stat(dbdir.c_str(), &st1);
    string rcldbdir;
    if (rcldb) 
	rcldbdir = rcldb->getDbDir();
    stat(rcldbdir.c_str(), &st2);
    path_catslash(rcldbdir);
    fprintf(stderr, "rcldbdir: [%s]\n", rcldbdir.c_str());
    if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) {
	QMessageBox::warning(0, "Recoll", 
			     tr("This is the main/local index!"));
	return;
    }
    if (allDbsLB->findItem(extraDbLE->text(), 
#if QT_VERSION < 0x040000
			    Qt::CaseSensitive|Qt::ExactMatch
#else
			    Q3ListBox::CaseSensitive|Q3ListBox::ExactMatch
#endif
				    )) {
	QMessageBox::warning(0, "Recoll", 
		 tr("The selected directory is already in the index list"));
	return;
    }
    allDbsLB->insertItem(extraDbLE->text());
    allDbsLB->sort();
}

void UIPrefsDialog::browseDbPB_clicked()
{
    QFileDialog fdia;
    bool savedh = fdia.showHiddenFiles();
    fdia.setShowHiddenFiles(true);
    QString s = QFileDialog::getExistingDirectory("", this, 0, 
       tr("Select xapian index directory (ie: /home/buddy/.recoll/xapiandb)"));

    fdia.setShowHiddenFiles(savedh);
    if (!s.isEmpty()) 
	extraDbLE->setText(s);
}
