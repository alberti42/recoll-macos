#ifndef lint
static char rcsid[] = "@(#$Id: sort_w.cpp,v 1.1 2006-09-04 15:13:01 dockes Exp $ (C) 2006 J.F.Dockes";
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

#include <qcombobox.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "sortseq.h"
#include "debuglog.h"
#include "sort_w.h"

void SortForm::init()
{
    const char *labels[5];
    labels[0] = "";
    labels[1] = "Date";
    labels[2] = "Mime type";
    labels[3] = 0;
    fldCMB1->insertStrList(labels, 3);
    fldCMB1->setCurrentItem(0);
    fldCMB2->insertStrList(labels, 3); fldCMB2->setCurrentItem(0);
    // signals and slots connections
    connect(resetPB, SIGNAL(clicked()), this, SLOT(reset()));
    connect(closePB, SIGNAL(clicked()), this, SLOT(close()));
    connect(mcntSB, SIGNAL(valueChanged(int)), this, SLOT(setData()));
    connect(fldCMB1, SIGNAL(activated(const QString&)), this, SLOT(setData()));
    connect(fldCMB2, SIGNAL(activated(const QString&)), this, SLOT(setData()));
    connect(descCB1, SIGNAL(stateChanged(int)), this, SLOT(setData()));
    connect(descCB2, SIGNAL(stateChanged(int)), this, SLOT(setData()));
    connect(sortCB, SIGNAL(toggled(bool)), this, SLOT(setData()));
}

void SortForm::reset()
{
    mcntSB->setValue(100);
    fldCMB1->setCurrentItem(0);
    fldCMB2->setCurrentItem(0);
    descCB1->setChecked(false);
    descCB1->setChecked(false);
    sortCB->setChecked(false);
    setData();
}

void SortForm::setData()
{
    LOGDEB(("SortForm::setData\n"));
    RclSortSpec spec;

    mcntSB->setEnabled(sortCB->isChecked());
    fldCMB1->setEnabled(sortCB->isChecked());
    descCB1->setEnabled(sortCB->isChecked());
    fldCMB2->setEnabled(sortCB->isChecked());
    descCB2->setEnabled(sortCB->isChecked());

    if (!sortCB->isChecked()) {
	spec.sortwidth = 0;
    } else {
	bool desc = descCB1->isChecked();
	switch (fldCMB1->currentItem()) {
	case 1: 
	    spec.addCrit(RclSortSpec::RCLFLD_MTIME, desc?true:false);
	    break;
	case 2: 
	    spec.addCrit(RclSortSpec::RCLFLD_MIMETYPE, desc?true:false);
	    break;
	}

	desc = descCB2->isChecked();
	switch (fldCMB2->currentItem()) {
	case 1: 
	    spec.addCrit(RclSortSpec::RCLFLD_MTIME, desc?true:false);
	    break;
	case 2: 
	    spec.addCrit(RclSortSpec::RCLFLD_MIMETYPE, desc?true:false);
	    break;
	}
	spec.sortwidth = mcntSB->value();
    }
    emit sortDataChanged(spec);
}
