/* @(#$Id: ssearch_w.h,v 1.1 2006-09-04 15:13:02 dockes Exp $  (C) 2006 J.F.Dockes */
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
#ifndef _SSEARCH_W_H_INCLUDED_
#define _SSEARCH_W_H_INCLUDED_

#include <qvariant.h>
#include <qwidget.h>
#include "recoll.h"
#include "searchdata.h"
#include "ssearchb.h"

class SSearch : public SSearchBase
{
    Q_OBJECT

public:
    SSearch( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 )
	: SSearchBase(parent,name,fl) {init();}
    ~SSearch(){}

    virtual void init();
    virtual void setAnyTermMode();
    virtual void completion();
    virtual bool event( QEvent * evt );

public slots:
    virtual void searchTextChanged( const QString & text );
    virtual void startSimpleSearch();

signals:
    void startSearch(Rcl::AdvSearchData);
    void clearSearch();
};


#endif /* _SSEARCH_W_H_INCLUDED_ */
