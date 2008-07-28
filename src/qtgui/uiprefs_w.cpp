#ifndef lint
static char rcsid[] = "@(#$Id: uiprefs_w.cpp,v 1.25 2008-07-28 08:42:52 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <qlistview.h>
#include <qfiledialog.h>
#else
#include <q3listbox.h>
#include <q3listview.h>
#include <q3filedialog.h>
#define QListView Q3ListView
#define QCheckListItem Q3CheckListItem
#define QFileDialog  Q3FileDialog
#define QListViewItemIterator Q3ListViewItemIterator
#endif
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtextedit.h>

#include "recoll.h"
#include "guiutils.h"
#include "rcldb.h"
#include "rclconfig.h"
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

    connect(addExtraDbPB, SIGNAL(clicked()), 
	    this, SLOT(addExtraDbPB_clicked()));
    connect(delExtraDbPB, SIGNAL(clicked()), 
	    this, SLOT(delExtraDbPB_clicked()));
    connect(togExtraDbPB, SIGNAL(clicked()), 
	    this, SLOT(togExtraDbPB_clicked()));
    connect(actAllExtraDbPB, SIGNAL(clicked()), 
	    this, SLOT(actAllExtraDbPB_clicked()));
    connect(unacAllExtraDbPB, SIGNAL(clicked()), 
	    this, SLOT(unacAllExtraDbPB_clicked()));

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
    maxHLTSB->setValue(prefs.maxhltextmbs);
    autoSearchCB->setChecked(prefs.autoSearchOnWS);
    syntlenSB->setValue(prefs.syntAbsLen);
    syntctxSB->setValue(prefs.syntAbsCtx);

    initStartAdvCB->setChecked(prefs.startWithAdvSearchOpen);
    initStartSortCB->setChecked(prefs.startWithSortToolOpen);
    useDesktopOpenCB->setChecked(prefs.useDesktopOpen);
    keepSortCB->setChecked(prefs.keepSort);

    // Query terms color
    qtermColorLE->setText(prefs.qtermcolor);
    
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
    stemLangCMB->insertItem(g_stringNoStem);
    stemLangCMB->insertItem(g_stringAllStem);
    list<string> langs;
    if (!getStemLangs(langs)) {
	QMessageBox::warning(0, "Recoll", 
			     tr("error retrieving stemming languages"));
    }
    int cur = prefs.queryStemLang == ""  ? 0 : 1;
    for (list<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	stemLangCMB->
	    insertItem(QString::fromAscii(it->c_str(), it->length()));
	if (cur == 0 && !strcmp(prefs.queryStemLang.ascii(), it->c_str())) {
	    cur = stemLangCMB->count();
	}
    }
    stemLangCMB->setCurrentItem(cur);

    autoPhraseCB->setChecked(prefs.ssearchAutoPhrase);

    buildAbsCB->setChecked(prefs.queryBuildAbstract);
    replAbsCB->setEnabled(prefs.queryBuildAbstract);
    replAbsCB->setChecked(prefs.queryReplaceAbstract);

    // Initialize the extra indexes listboxes
    idxLV->clear();
    for (list<string>::iterator it = prefs.allExtraDbs.begin(); 
	 it != prefs.allExtraDbs.end(); it++) {
	QCheckListItem *item = 
	    new QCheckListItem(idxLV, QString::fromLocal8Bit(it->c_str()), 
			       QCheckListItem::CheckBox);
	if (item) item->setOn(false);
    }
    for (list<string>::iterator it = prefs.activeExtraDbs.begin(); 
	 it != prefs.activeExtraDbs.end(); it++) {
	QCheckListItem *item;
	if ((item = (QCheckListItem *)
	     idxLV->findItem (QString::fromLocal8Bit(it->c_str()), 0))) {
	    item->setOn(true);
	}
    }
}

void UIPrefsDialog::accept()
{
    prefs.autoSearchOnWS = autoSearchCB->isChecked();
    prefs.respagesize = pageLenSB->value();
    prefs.maxhltextmbs = maxHLTSB->value();

    prefs.qtermcolor = qtermColorLE->text();
    prefs.reslistfontfamily = reslistFontFamily;
    prefs.reslistfontsize = reslistFontSize;
    prefs.reslistformat =  rlfTE->text();
    // Don't let us set the old default format from here, this would
    // get reset to the new default. Ugly hack
    if (prefs.reslistformat == 
	QString::fromAscii(prefs.getV18DfltResListFormat())) {
	prefs.reslistformat += " ";
	rlfTE->setText(prefs.reslistformat);
    }
    if (prefs.reslistformat.stripWhiteSpace().isEmpty()) {
	prefs.reslistformat = prefs.getDfltResListFormat();
	rlfTE->setText(prefs.reslistformat);
    }

    prefs.htmlBrowser =  helpBrowserLE->text();

    if (stemLangCMB->currentItem() == 0) {
	prefs.queryStemLang = "";
    } else if (stemLangCMB->currentItem() == 1) {
	prefs.queryStemLang = "ALL";
    } else {
	prefs.queryStemLang = stemLangCMB->currentText();
    }
    prefs.ssearchAutoPhrase = autoPhraseCB->isChecked();
    prefs.queryBuildAbstract = buildAbsCB->isChecked();
    prefs.queryReplaceAbstract = buildAbsCB->isChecked() && 
	replAbsCB->isChecked();

    prefs.startWithAdvSearchOpen = initStartAdvCB->isChecked();
    prefs.startWithSortToolOpen = initStartSortCB->isChecked();
    prefs.useDesktopOpen = useDesktopOpenCB->isChecked();
    prefs.keepSort = keepSortCB->isChecked();

    prefs.syntAbsLen = syntlenSB->value();
    prefs.syntAbsCtx = syntctxSB->value();

    QListViewItemIterator it(idxLV);
    prefs.allExtraDbs.clear();
    prefs.activeExtraDbs.clear();
    while (it.current()) {
	QCheckListItem *item = (QCheckListItem *)it.current();
	prefs.allExtraDbs.push_back((const char *)item->text().local8Bit());
	if (item->isOn()) {
	    prefs.activeExtraDbs.push_back((const char *)
					   item->text().local8Bit());
	}
	++it;
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

void UIPrefsDialog::setStemLang(const QString& lang)
{
    int cur = 0;
    if (lang == "") {
	cur = 0;
    } else if (lang == "ALL") {
	cur = 1;
    } else {
	for (int i = 1; i < stemLangCMB->count(); i++) {
	    if (lang == stemLangCMB->text(i)) {
		cur = i;
		break;
	    }
	}
    }
    stemLangCMB->setCurrentItem(cur);
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
    } else {
	// Close and reopen, in hope that makes us visible...
	m_viewAction->close();
    }
    m_viewAction->show();
}

////////////////////////////////////////////
// External / extra search indexes setup
// TBD: a way to remove entry from 'all' list (del button?)

void UIPrefsDialog::togExtraDbPB_clicked()
{
    QListViewItemIterator it(idxLV);
    while (it.current()) {
	QCheckListItem *item = (QCheckListItem *)it.current();
	if (item->isSelected()) {
	    item->setOn(!item->isOn());
	}
	++it;
    }
}
void UIPrefsDialog::actAllExtraDbPB_clicked()
{
    QListViewItemIterator it(idxLV);
    while (it.current()) {
	QCheckListItem *item = (QCheckListItem *)it.current();
	item->setOn(true);
	++it;
    }
}
void UIPrefsDialog::unacAllExtraDbPB_clicked()
{
    QListViewItemIterator it(idxLV);
    while (it.current()) {
	QCheckListItem *item = (QCheckListItem *)it.current();
	item->setOn(false);
	++it;
    }
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
    string rcldbdir = RclConfig::getMainConfig()->getDbDir();
    stat(rcldbdir.c_str(), &st2);
    path_catslash(rcldbdir);
    fprintf(stderr, "rcldbdir: [%s]\n", rcldbdir.c_str());
    if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) {
	QMessageBox::warning(0, "Recoll", 
			     tr("This is the main/local index!"));
	return;
    }
    if (idxLV->findItem(extraDbLE->text(), 
#if QT_VERSION < 0x040000
			    Qt::CaseSensitive|Qt::ExactMatch
#else
			    Q3ListView::CaseSensitive|Q3ListView::ExactMatch
#endif
				    )) {
	QMessageBox::warning(0, "Recoll", 
		 tr("The selected directory is already in the index list"));
	return;
    }
    new QCheckListItem(idxLV, extraDbLE->text(), QCheckListItem::CheckBox);
    idxLV->sort();
}

void UIPrefsDialog::delExtraDbPB_clicked()
{
    list<QCheckListItem*> dlt;
    QListViewItemIterator it(idxLV);
    while (it.current()) {
	QCheckListItem *item = (QCheckListItem *)it.current();
	if (item->isSelected()) {
	    dlt.push_back(item);
	}
	++it;
    }
    for (list<QCheckListItem*>::iterator it = dlt.begin(); 
	 it != dlt.end(); it++) 
	delete *it;
}

void UIPrefsDialog::extraDbTextChanged(const QString &text)
{
    if (text.isEmpty()) {
	addExtraDbPB->setEnabled(false);
    } else {
	addExtraDbPB->setEnabled(true);
    }
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
