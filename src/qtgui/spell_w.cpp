#ifndef lint
static char rcsid[] = "@(#$Id: spell_w.cpp,v 1.1 2006-10-11 14:16:26 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
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
#include "autoconfig.h"

#include <unistd.h>

#include <list>

#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>

#include "debuglog.h"
#include "recoll.h"
#include "spell_w.h"

#ifdef RCL_USE_ASPELL
#include "rclaspell.h"
#endif

void SpellW::init()
{
    // signals and slots connections
    connect(baseWordLE, SIGNAL(textChanged(const QString&)), 
	    this, SLOT(wordChanged(const QString&)));
    connect(baseWordLE, SIGNAL(returnPressed()), this, SLOT(doExpand()));
    connect(expandPB, SIGNAL(clicked()), this, SLOT(doExpand()));
    connect(clearPB, SIGNAL(clicked()), baseWordLE, SLOT(clear()));
    connect(dismissPB, SIGNAL(clicked()), this, SLOT(close()));
}

void SpellW::doExpand()
{
#ifdef RCL_USE_ASPELL
    string reason;
    if (!aspell || !maybeOpenDb(reason)) {
	LOGDEB(("SpellW::doExpand: error aspell %p db: %s\n", aspell,
		reason.c_str()));
	return;
    }
    if (!baseWordLE->text().isEmpty()) {
	list<string> suggs;
	string word = string((const char *)baseWordLE->text().utf8());
	if (!aspell->suggest(*rcldb, word, suggs, reason)) {
	    LOGERR(("SpellW::doExpand:suggest failed: %s\n", reason.c_str()));
	    return;
	}
	suggsTE->clear();
	if (suggs.empty()) {
	    suggsTE->append(tr("No spelling expansion found"));
	} else {
	    for (list<string>::iterator it = suggs.begin(); 
		 it != suggs.end(); it++) {
		suggsTE->append(QString::fromUtf8(it->c_str()));
	    }
	    suggsTE->setCursorPosition(0,0);
	    suggsTE->ensureCursorVisible();
	}
    }
#endif
}

void SpellW::wordChanged(const QString &text)
{
    if (text.isEmpty()) {
	expandPB->setEnabled(false);
	clearPB->setEnabled(false);
	suggsTE->clear();
    } else {
	expandPB->setEnabled(true);
	clearPB->setEnabled(true);
    }
}
