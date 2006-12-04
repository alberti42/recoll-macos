/* @(#$Id: sort_w.h,v 1.4 2006-12-04 09:56:26 dockes Exp $  (C) 2005 J.F.Dockes */
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
#ifndef _SORT_W_H_INCLUDED_
#define _SORT_W_H_INCLUDED_

#include <qvariant.h>
#include <qdialog.h>
#include "sortseq.h"
#if (QT_VERSION < 0x040000)
#include "sort.h"
#else
#include "ui_sort.h"
#endif

class QDialog;

//MOC_SKIP_BEGIN
#if QT_VERSION < 0x040000
class DummySortFormBase : public SortFormBase
{
 public: DummySortFormBase(QWidget* parent = 0) : SortFormBase(parent) {}
};
#else
class DummySortFormBase : public QDialog, public Ui::SortFormBase
{
public: DummySortFormBase(QWidget* parent):QDialog(parent){setupUi(this);}
};
#endif
//MOC_SKIP_END

class SortForm : public DummySortFormBase
{
    Q_OBJECT

public:
    SortForm(QDialog* parent = 0) 
	: DummySortFormBase(parent) 
    {
	init();
    }
    ~SortForm() {}


public slots:
    virtual void reset();
    virtual void setData();

signals:
    void sortDataChanged(DocSeqSortSpec);

private:
    virtual void init();
};


#endif /* _SORT_W_H_INCLUDED_ */
