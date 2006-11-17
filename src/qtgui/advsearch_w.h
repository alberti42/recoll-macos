#ifndef _ADVSEARCH_W_H_INCLUDED_
#define _ADVSEARCH_W_H_INCLUDED_
/* @(#$Id: advsearch_w.h,v 1.7 2006-11-17 15:26:40 dockes Exp $  (C) 2005 J.F.Dockes */
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
#include <list>
#include <qvariant.h>
#include <qdialog.h>
#include "advsearch.h"
#include "searchclause_w.h"
#include "recoll.h"
#include "refcntr.h"
#include "searchdata.h"

class AdvSearch : public AdvSearchBase
{
    Q_OBJECT

public:
    AdvSearch(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, 
	      WFlags fl = 0) 
	: AdvSearchBase(parent,name,modal,fl)
    {init();}
    ~AdvSearch(){}

public slots:
    virtual void delFiltypPB_clicked();
    virtual void delAFiltypPB_clicked();
    virtual void addFiltypPB_clicked();
    virtual void addAFiltypPB_clicked();
    virtual void restrictFtCB_toggled(bool on);
    virtual void searchPB_clicked();
    virtual void browsePB_clicked();
    virtual void saveFileTypes();
    virtual void delClause();
    virtual void addClause();
    virtual void addClause(int);
    virtual bool close();
    virtual void polish();

signals:
    void startSearch(RefCntr<Rcl::SearchData>);

private:
    virtual void init();
    std::list<SearchClauseW *> m_clauseWins;
    void saveCnf();
};


#endif /* _ADVSEARCH_W_H_INCLUDED_ */
