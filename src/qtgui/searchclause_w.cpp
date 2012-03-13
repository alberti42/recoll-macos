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

#include "recoll.h"

#include "searchclause_w.h"

#include <qvariant.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a SearchClauseW as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
SearchClauseW::SearchClauseW(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* hLayout = new QHBoxLayout(this); 

    sTpCMB = new QComboBox(this);
    sTpCMB->setEditable(false);
    hLayout->addWidget(sTpCMB);

    fldCMB = new QComboBox(this);
    fldCMB->setEditable(false);
    hLayout->addWidget(fldCMB);

    proxSlackSB = new QSpinBox(this);
    hLayout->addWidget(proxSlackSB);

    wordsLE = new QLineEdit(this);
    wordsLE->setMinimumSize(QSize(190, 0));
    hLayout->addWidget(wordsLE);

    languageChange();
    resize(QSize(0, 0).expandedTo(minimumSizeHint()));

    connect(sTpCMB, SIGNAL(activated(int)), this, SLOT(tpChange(int)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
SearchClauseW::~SearchClauseW()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void SearchClauseW::languageChange()
{
    sTpCMB->clear();
    sTpCMB->addItem(tr("Any")); // 0
    sTpCMB->addItem(tr("All")); //1
    sTpCMB->addItem(tr("None"));//2
    sTpCMB->addItem(tr("Phrase"));//3
    sTpCMB->addItem(tr("Proximity"));//4
    sTpCMB->addItem(tr("File name"));//5
    //    sTpCMB->insertItem(tr("Complex clause"));//6

    fldCMB->addItem(tr("No field"));
    if (theconfig) {
	set<string> fields = theconfig->getIndexedFields();
	for (set<string>::const_iterator it = fields.begin(); 
	     it != fields.end(); it++) {
	    // Some fields don't make sense here
	    if (it->compare("filename")) {
		fldCMB->addItem(QString::fromUtf8(it->c_str()));
	    }
	}
    }
    // Ensure that the spinbox will be enabled/disabled depending on
    // combobox state
    tpChange(0);

    sTpCMB->setToolTip(tr("Select the type of query that will be performed with the words"));
    proxSlackSB->setToolTip(tr("Number of additional words that may be interspersed with the chosen ones"));
}

using namespace Rcl;

// Translate my window state into an Rcl search clause
SearchDataClause *
SearchClauseW::getClause()
{
    if (wordsLE->text().isEmpty())
	return 0;
    string field;
    if (fldCMB->currentIndex() != 0) {
	field = (const char *)fldCMB->currentText().toUtf8();
    }
    string text = (const char *)wordsLE->text().toUtf8();
    switch (sTpCMB->currentIndex()) {
    case 0:
	return new SearchDataClauseSimple(SCLT_OR, text, field);
    case 1:
	return new SearchDataClauseSimple(SCLT_AND, text, field);
    case 2:
	return new SearchDataClauseSimple(SCLT_EXCL, text, field);
    case 3:
	return new SearchDataClauseDist(SCLT_PHRASE, text, 
					proxSlackSB->value(), field);
    case 4:
	return new SearchDataClauseDist(SCLT_NEAR, text,
					proxSlackSB->value(), field);
    case 5:
	return new SearchDataClauseFilename(text);
    case 6:
    default:
	return 0;
    }
}

// Handle combobox change: may need to enable/disable the distance
// spinbox and field spec
void SearchClauseW::tpChange(int index)
{
    if (index < 0 || index > 5)
	return;
    if (sTpCMB->currentIndex() != index)
	sTpCMB->setCurrentIndex(index);
    switch (index) {
    case 3:
    case 4:
	proxSlackSB->show();
	proxSlackSB->setEnabled(true);
	if (index == 4)
	    proxSlackSB->setValue(10);
	break;
    default:
	proxSlackSB->close();
    }
    if (index == 5) {
	fldCMB->close();
    } else {
	fldCMB->show();
    }
}
