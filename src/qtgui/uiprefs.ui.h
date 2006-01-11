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

#include "qfontdialog.h"
#include "qspinbox.h"
#include "qmessagebox.h"

#include "recoll.h"

void UIPrefsDialog::init()
{
    // Entries per result page spinbox
    pageLenSB->setValue(prefs_respagesize);
    // Show icons checkbox
    useIconsCB->setChecked(prefs_showicons);
    // Result list font family and size
    reslistFontFamily = prefs_reslistfontfamily;
    reslistFontSize = prefs_reslistfontsize;
    QString s;
    if (prefs_reslistfontfamily.length() == 0) {
	reslistFontPB->setText(this->font().family() + "-" +
			   s.setNum(this->font().pointSize()));
    } else {
	reslistFontPB->setText(reslistFontFamily + "-" +
			   s.setNum(reslistFontSize));
    }
    // Stemming language combobox
    stemLangCMB->insertItem("(no stemming)");
    list<string> langs;
#if 0
    string slangs;
    if (rclconfig->getConfParam("indexstemminglanguages", slangs)) {
	stringToStrings(slangs, langs);
    }
#else
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }
    langs = rcldb->getStemLangs();
#endif

    int i = 0, cur = -1;
    for (list<string>::const_iterator it = langs.begin(); 
	 it != langs.end(); it++) {
	stemLangCMB->
	    insertItem(QString::fromAscii(it->c_str(), it->length()));
	i++;
	if (cur == -1) {
	    if (!strcmp(prefs_queryStemLang.ascii(), it->c_str()))
		cur = i;
	}
    }
    if (cur < 0)
	cur = 0;
	stemLangCMB->setCurrentItem(cur);
    
    connect(reslistFontPB, SIGNAL(clicked()), this, SLOT(showFontDialog()));
    connect(resetFontPB, SIGNAL(clicked()), this, SLOT(resetReslistFont()));
}

void UIPrefsDialog::accept()
{
    emit uiprefsDone();
    QDialog::accept();
}

void UIPrefsDialog::showFontDialog()
{
    bool ok;
    QFont font;
    if (prefs_reslistfontfamily.length()) {
	font.setFamily(prefs_reslistfontfamily);
	font.setPointSize(prefs_reslistfontsize);
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
