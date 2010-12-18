#ifndef _RESTABLE_H_INCLUDED_
#define _RESTABLE_H_INCLUDED_
/* @(#$Id: spell_w.h,v 1.7 2006-12-22 11:01:28 dockes Exp $  (C) 2006 J.F.Dockes */
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

#include "ui_restable.h"

class ResTable : public Ui::ResTable
{
    Q_OBJECT
public:
    ResTable(QWidget* parent = 0) 
	: QWidget(parent) 
    {
	setupUi(this);
	init();
    }
	
    ~ResTable(){}

public slots:

signals:

private:
    void init();
};


#endif /* _RESTABLE_H_INCLUDED_ */
