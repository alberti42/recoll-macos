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

#include "sortseq.h"
#include "debuglog.h"

void SortForm::init()
{
    const char *labels[5];
    labels[0] = "";
    labels[1] = "Date";
    labels[2] = "Mime type";
    labels[3] = 0;
    fldCMB1->insertStrList(labels, 3);
    fldCMB1->setCurrentItem(0);
    fldCMB2->insertStrList(labels, 3);
    fldCMB2->setCurrentItem(0);
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
    int width;

    mcntSB->setEnabled(sortCB->isChecked());
    fldCMB1->setEnabled(sortCB->isChecked());
    descCB1->setEnabled(sortCB->isChecked());
    fldCMB2->setEnabled(sortCB->isChecked());
    descCB2->setEnabled(sortCB->isChecked());

    if (!sortCB->isChecked()) {
	width = 0;
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
	width = mcntSB->value();
    }
    emit sortDataChanged(width, spec);
}
