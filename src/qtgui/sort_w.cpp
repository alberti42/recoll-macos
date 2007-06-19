#ifndef lint
static char rcsid[] = "@(#$Id: sort_w.cpp,v 1.6 2007-06-19 16:19:24 dockes Exp $ (C) 2006 J.F.Dockes";
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
#include "guiutils.h"

#include "sort_w.h"

void SortForm::init()
{
    QStringList slabs;
    slabs += QString();
    slabs += tr("Date");
    slabs += tr("Mime type");

#if QT_VERSION < 0x040000
    fldCMB1->insertStringList(slabs);
    fldCMB2->insertStringList(slabs); 
#else
    fldCMB1->addItems(slabs);
    fldCMB2->addItems(slabs); 
#endif

    // Initialize values from prefs:
    mcntSB->setValue(prefs.sortWidth);
    unsigned int spec = (unsigned int)prefs.sortSpec;

    // Restore active/inactive state from prefs, only if requested
    if (prefs.keepSort) {
	sortCB->setChecked(prefs.sortActive);
    } else {
	sortCB->setChecked(false);
    }

    // We use 4 bits per spec hi is direction, 3 low bits = sort field
    unsigned int v, d;

    v = spec & (0xf & ~(1<<3));
    d = spec & (1 << 3);
    spec >>= 4;
    fldCMB1->setCurrentItem(v < 3 ? v : 0);
    descCB1->setChecked(d != 0 ? true : false);

    v = spec & (0xf & ~(1<<3));
    d = spec & (1 << 3);
    spec >>= 4;
    fldCMB2->setCurrentItem(v < 3 ? v : 0);
    descCB2->setChecked(d !=0 ? true : false);

    // signals and slots connections
    connect(applyPB, SIGNAL(clicked()), this, SLOT(apply()));
    connect(closePB, SIGNAL(clicked()), this, SLOT(close()));
    connect(mcntSB, SIGNAL(valueChanged(int)), this, SLOT(setData()));
    connect(fldCMB1, SIGNAL(activated(const QString&)), this, SLOT(setData()));
    connect(fldCMB2, SIGNAL(activated(const QString&)), this, SLOT(setData()));
    connect(descCB1, SIGNAL(stateChanged(int)), this, SLOT(setData()));
    connect(descCB2, SIGNAL(stateChanged(int)), this, SLOT(setData()));
    connect(sortCB, SIGNAL(toggled(bool)), this, SLOT(setData()));

    // Finalize state
    setData();
}

// This is called when the 'apply' button is pressed. We 
void SortForm::apply()
{
    setData();
    emit applySortData();
}

void SortForm::setData()
{
    LOGDEB(("SortForm::setData\n"));
    DocSeqSortSpec spec;

    mcntSB->setEnabled(sortCB->isChecked());
    fldCMB1->setEnabled(sortCB->isChecked());
    descCB1->setEnabled(sortCB->isChecked());
    fldCMB2->setEnabled(sortCB->isChecked());
    descCB2->setEnabled(sortCB->isChecked());

    prefs.sortActive = sortCB->isChecked();

    if (!sortCB->isChecked()) {
	spec.sortwidth = 0;
    } else {
	bool desc = descCB1->isChecked();
	switch (fldCMB1->currentItem()) {
	case 1: 
	    spec.addCrit(DocSeqSortSpec::RCLFLD_MTIME, desc?true:false);
	    break;
	case 2: 
	    spec.addCrit(DocSeqSortSpec::RCLFLD_MIMETYPE, desc?true:false);
	    break;
	}

	desc = descCB2->isChecked();
	switch (fldCMB2->currentItem()) {
	case 1: 
	    spec.addCrit(DocSeqSortSpec::RCLFLD_MTIME, desc?true:false);
	    break;
	case 2: 
	    spec.addCrit(DocSeqSortSpec::RCLFLD_MIMETYPE, desc?true:false);
	    break;
	}
	spec.sortwidth = mcntSB->value();

	// Save data to prefs;
	prefs.sortWidth = spec.sortwidth;
	unsigned int spec = 0, v, d;
	v = fldCMB1->currentItem() & 0x7;
	d = descCB1->isChecked() ? 8 : 0;
	spec |=  (d|v);
	v = fldCMB2->currentItem() & 0x7;
	d = descCB2->isChecked() ? 8 : 0;
	spec |= (d|v) << 4;
	prefs.sortSpec = (int) spec;
    }
    emit sortDataChanged(spec);
}
