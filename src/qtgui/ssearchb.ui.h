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

#include "debuglog.h"

void SSearchBase::init()
{
    searchTypCMB->insertItem(tr("Any term"));
    searchTypCMB->insertItem(tr("All terms"));
    searchTypCMB->insertItem(tr("File name"));
}

void SSearchBase::searchTextChanged( const QString & text )
{
    if (text.isEmpty()) {
	searchPB->setEnabled(false);
	clearqPB->setEnabled(false);
    } else {
	searchPB->setEnabled(true);
	clearqPB->setEnabled(true);
    }
}

void SSearchBase::startSimpleSearch()
{
    LOGDEB(("SSearchBase::startSimpleSearch\n"));

    Rcl::AdvSearchData sdata;
    QCString u8 =  queryText->text().utf8();
    switch (searchTypCMB->currentItem()) {
    case 0:
    default:
	sdata.orwords = u8;
	break;
    case 1:
	sdata.allwords = u8;
	break;
    case 2:
	sdata.filename = u8;
	break;
    }

    emit startSearch(sdata);
}
