#ifndef lint
static char rcsid[] = "@(#$Id: reslist.cpp,v 1.52 2008-12-17 15:12:08 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include "autoconfig.h"

#include <stdlib.h>

#include "restable.h"

class RecollModel : public QAbstractTableModel {
    Q_OBJECT

public:
    RecollModel(QObject *parent = 0)
	: QAbstractTableModel(parent)
    {
    }

private:

};

void ResTable::init()
{

// Set up the columns according to the list of fields we are displaying
}
