#ifndef _ADVSEARCH_W_H_INCLUDED_
#define _ADVSEARCH_W_H_INCLUDED_
/* @(#$Id: advsearch_w.h,v 1.11 2006-12-20 13:12:49 dockes Exp $  (C) 2005 J.F.Dockes */
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

#include "searchclause_w.h"
#include "recoll.h"
#include "refcntr.h"
#include "searchdata.h"

class QDialog;

//MOC_SKIP_BEGIN
#if QT_VERSION < 0x040000

#include "advsearch.h"
class DummyAdvSearchBase : public AdvSearchBase
{
 public: DummyAdvSearchBase(QWidget* parent = 0) : AdvSearchBase(parent) {}
};

#else

#include "ui_advsearch.h"
class DummyAdvSearchBase : public QDialog, public Ui::AdvSearchBase
{
public: DummyAdvSearchBase(QWidget *parent):QDialog(parent) {setupUi(this);}
};

#endif
//MOC_SKIP_END

class AdvSearch : public DummyAdvSearchBase
{
    Q_OBJECT

public:
    AdvSearch(QDialog* parent = 0) 
	: DummyAdvSearchBase(parent)
    {
	init();
    }
    ~AdvSearch(){}

public slots:
    virtual void delFiltypPB_clicked();
    virtual void delAFiltypPB_clicked();
    virtual void addFiltypPB_clicked();
    virtual void addAFiltypPB_clicked();
    virtual void restrictFtCB_toggled(bool);
    virtual void restrictCtCB_toggled(bool);
    virtual void searchPB_clicked();
    virtual void browsePB_clicked();
    virtual void saveFileTypes();
    virtual void delClause();
    virtual void addClause();
    virtual void addClause(int);
    virtual bool close();
#if (QT_VERSION < 0x040000)
    virtual void polish();
#endif

signals:
    void startSearch(RefCntr<Rcl::SearchData>);

private:
    virtual void init();
    std::list<SearchClauseW *> m_clauseWins;
    void saveCnf();
    void fillFileTypes();
};


#endif /* _ADVSEARCH_W_H_INCLUDED_ */
