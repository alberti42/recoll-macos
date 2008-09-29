#ifndef lint
static char rcsid[] = "@(#$Id: docseq.cpp,v 1.11 2008-09-29 08:59:20 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <math.h>
#include <time.h>

#include "docseq.h"

int DocSequence::getSeqSlice(int offs, int cnt, vector<ResListEntry>& result)
{
    int ret = 0;
    for (int num = offs; num < offs + cnt; num++, ret++) {
	result.push_back(ResListEntry());
	if (!getDoc(num, result.back().doc, &result.back().subHeader)) {
	    result.pop_back();
	    return ret;
	}
    }
    return ret;
}

