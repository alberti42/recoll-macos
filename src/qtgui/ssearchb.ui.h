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
    LOGDEB(("RclMain::queryText_returnPressed()\n"));
    // The db may have been closed at the end of indexing
    Rcl::AdvSearchData sdata;

    QCString u8 =  queryText->text().utf8();
    if (allTermsCB->isChecked())
	sdata.allwords = u8;
    else
	sdata.orwords = u8;

    emit startSearch(sdata);
}
