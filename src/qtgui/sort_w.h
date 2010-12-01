/* @(#$Id: sort_w.h,v 1.5 2006-12-05 15:23:50 dockes Exp $  (C) 2005 J.F.Dockes */
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
#include "ui_sort.h"

class QDialog;

class SortForm : public QDialog, public Ui::SortFormBase
{
    Q_OBJECT

public:
    SortForm(QWidget* parent = 0) 
	: QDialog(parent) 
    {
	setupUi(this);
	init();
    }
    ~SortForm() {}

public slots:
    virtual void apply();
    virtual void setData();

signals:
    void sortDataChanged(DocSeqSortSpec);
    void applySortData();
private:
    virtual void init();
};


#endif /* _SORT_W_H_INCLUDED_ */
