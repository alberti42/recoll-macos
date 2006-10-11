#ifndef _ASPELL_W_H_INCLUDED_
#define _ASPELL_W_H_INCLUDED_
/* @(#$Id: spell_w.h,v 1.1 2006-10-11 14:16:26 dockes Exp $  (C) 2006 J.F.Dockes */
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

#include <qvariant.h>
#include <qwidget.h>
#include "rcldb.h"
#include "spell.h"

class SpellW : public SpellBase
{
    Q_OBJECT

public:
    SpellW(QWidget* parent = 0, const char* name = 0, WFlags fl = 0) :
	SpellBase(parent,name,fl) {init();}
	
    ~SpellW(){}

public slots:
    virtual void doExpand();
    virtual void wordChanged(const QString&);
    //    virtual void textDoubleClicked(int, int);

signals:
    //    void wordSelect(QString);

private:
    void init();
};

#endif /* _ASPELL_W_H_INCLUDED_ */
