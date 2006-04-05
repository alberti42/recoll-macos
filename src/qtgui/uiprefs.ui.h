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
#include <string>
#include <algorithm>
#include <list>

#include "qfontdialog.h"
#include "qfiledialog.h"
#include "qspinbox.h"
#include "qmessagebox.h"

#include "recoll.h"
#include "guiutils.h"
#include "rcldb.h"

void UIPrefsDialog::init()
{
    // Entries per result page spinbox
    pageLenSB->setValue(prefs.respagesize);
    // Show icons checkbox
    useIconsCB->setChecked(prefs.showicons);
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
    helpBrowserLE->setText(prefs.htmlBrowser);
    // Stemming language combobox
    stemLangCMB->insertItem("(no stemming)");
    list<string> langs;
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }
    langs = rcldb->getStemLangs();

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

    buildAbsCB->setDown(prefs.queryBuildAbstract);
    if (!prefs.queryBuildAbstract) {
	replAbsCB->setEnabled(false);
    }
    replAbsCB->setDown(prefs.queryReplaceAbstract);

    // Initialize the extra databases listboxes
    QStringList ql;
    for (list<string>::iterator it = prefs.allExtraDbs.begin(); 
	 it != prefs.allExtraDbs.end(); it++) {
	ql.append(QString::fromLocal8Bit(it->c_str()));
    }
    allDbsLB->insertStringList(ql);
    ql.clear();
    for (list<string>::iterator it = prefs.activeExtraDbs.begin(); 
	 it != prefs.activeExtraDbs.end(); it++) {
	ql.append(QString::fromLocal8Bit(it->c_str()));
    }
    actDbsLB->insertStringList(ql);
    ql.clear();
    
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


}

void UIPrefsDialog::accept()
{
    prefs.showicons = useIconsCB->isChecked();
    prefs.respagesize = pageLenSB->value();

    prefs.reslistfontfamily = reslistFontFamily;
    prefs.reslistfontsize = reslistFontSize;

    prefs.htmlBrowser =  helpBrowserLE->text();

    if (stemLangCMB->currentItem() == 0) {
	prefs.queryStemLang = "";
    } else {
	prefs.queryStemLang = stemLangCMB->currentText();
    }
    prefs.queryBuildAbstract = buildAbsCB->isChecked();
    prefs.queryReplaceAbstract = buildAbsCB->isChecked() && 
	replAbsCB->isChecked();
    rwSettings(true);
    string reason;
    maybeOpenDb(reason, true);
    emit uiprefsDone();
    QDialog::accept();
}

void UIPrefsDialog::showFontDialog()
{
    bool ok;
    QFont font;
    if (prefs.reslistfontfamily.length()) {
	font.setFamily(prefs.reslistfontfamily);
	font.setPointSize(prefs.reslistfontsize);
    }

    font = QFontDialog::getFont(&ok, font, this );
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
					     "Choose a file" );
    if (s) 
	helpBrowserLE->setText(s);
}

////////////////////////////////////////////
// External / extra search databases setup: this should modify to take 
// effect only when Ok is clicked. Currently modifs take effect as soon as
// done in the Gui
// Also needed: means to remove entry from 'all' list (del button? )

void UIPrefsDialog::extraDbTextChanged(const QString &text)
{
    if (text.isEmpty()) {
	addExtraDbPB->setEnabled(false);
    } else {
	addExtraDbPB->setEnabled(true);
    }
}

// Add selected dbs to the active list
void UIPrefsDialog::addADbPB_clicked()
{
    for (unsigned int i = 0; i < allDbsLB->count();i++) {
	QListBoxItem *item = allDbsLB->item(i);
	if (item && item->isSelected()) {
	    allDbsLB->setSelected(i, false);
	    string dbname = (const char*)item->text().local8Bit();
	    if (std::find(prefs.activeExtraDbs.begin(), prefs.activeExtraDbs.end(), 
		     dbname) == prefs.activeExtraDbs.end()) {
		actDbsLB->insertItem(item->text());
		prefs.activeExtraDbs.push_back(dbname);
	    }
	}
    }
    actDbsLB->sort();
}

void UIPrefsDialog::addAADbPB_clicked()
{
    for (unsigned int i = 0; i < allDbsLB->count();i++) {
	allDbsLB->setSelected(i, true);
    }
    addADbPB_clicked();
}

void UIPrefsDialog::delADbPB_clicked()
{
    list<int> rmi;
    for (unsigned int i = 0; i < actDbsLB->count(); i++) {
	QListBoxItem *item = actDbsLB->item(i);
	if (item && item->isSelected()) {
	    string dbname = (const char*)item->text().local8Bit();
	    list<string>::iterator sit;
	    if ((sit = ::std::find(prefs.activeExtraDbs.begin(), 
			    prefs.activeExtraDbs.end(), dbname)) != 
		 prefs.activeExtraDbs.end()) {
		prefs.activeExtraDbs.erase(sit);
	    }
	    rmi.push_front(i);
	}
    }
    for (list<int>::iterator ii = rmi.begin(); ii != rmi.end(); ii++) {
	actDbsLB->removeItem(*ii);
    }
}

void UIPrefsDialog::delAADbPB_clicked()
{
    for (unsigned int i = 0; i < actDbsLB->count(); i++) {
	actDbsLB->setSelected(i, true);
    }
    delADbPB_clicked();
}

void UIPrefsDialog::addExtraDbPB_clicked()
{
    string dbdir = (const char *)extraDbLE->text().local8Bit();
    if (!Rcl::Db::testDbDir(dbdir)) {
	QMessageBox::warning(0, "Recoll", 
     tr("The selected directory does not appear to be a Xapian database"));
	return;
    }

    if (::std::find(prefs.allExtraDbs.begin(), prefs.allExtraDbs.end(), 
		    dbdir) != prefs.allExtraDbs.end()) {
	QMessageBox::warning(0, "Recoll", 
		 tr("The selected directory is already in the database list"));
	return;
    }
    prefs.allExtraDbs.push_back(dbdir);
    allDbsLB->insertItem(extraDbLE->text());
    allDbsLB->sort();
}

void UIPrefsDialog::browseDbPB_clicked()
{
    QFileDialog fdia;
    bool savedh = fdia.showHiddenFiles();
    fdia.setShowHiddenFiles(true);
    QString s = QFileDialog::getExistingDirectory("", this, 0, 
tr("Select directory holding xapian database (ie: /home/someone/.recoll/xapiandb)"));

    fdia.setShowHiddenFiles(savedh);
    if (s) 
	extraDbLE->setText(s);
}
