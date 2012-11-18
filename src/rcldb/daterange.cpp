/* The dates-to-query routine is is lifted quasi-verbatim but
 *  modified from xapian-omega:date.cc. Copyright info:
 *
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2001 James Aylett
 * Copyright 2001,2002 Ananova Ltd
 * Copyright 2002 Intercede 1749 Ltd
 * Copyright 2002,2003,2006 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#include "autoconfig.h"

#include <stdio.h>

#include <vector>
using namespace std;

#include "xapian.h"

#include "debuglog.h"
#include "rclconfig.h"

namespace Rcl {

#ifdef RCL_INDEX_STRIPCHARS
#define bufprefix(BUF, L) {(BUF)[0] = L;}
#define bpoffs() 1
#else
static inline void bufprefix(char *buf, char c)
{
    if (o_index_stripchars) {
	buf[0] = c;
    } else {
	buf[0] = ':'; 
	buf[1] = c; 
	buf[2] = ':';
    }
}
static inline int bpoffs() 
{
    return o_index_stripchars ? 1 : 3;
}
#endif

Xapian::Query date_range_filter(int y1, int m1, int d1, int y2, int m2, int d2)
{
    // Xapian uses a smallbuf and snprintf. Can't be bothered, we're
    // only doing %d's !
    char buf[200];
    bufprefix(buf, 'D');
    sprintf(buf+bpoffs(), "%04d%02d", y1, m1);
    vector<Xapian::Query> v;

    int d_last = monthdays(m1, y1);
    int d_end = d_last;
    if (y1 == y2 && m1 == m2 && d2 < d_last) {
	d_end = d2;
    }
    // Deal with any initial partial month
    if (d1 > 1 || d_end < d_last) {
    	for ( ; d1 <= d_end ; d1++) {
	    sprintf(buf + 6 + bpoffs(), "%02d", d1);
	    v.push_back(Xapian::Query(buf));
	}
    } else {
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }
    
    if (y1 == y2 && m1 == m2) {
	return Xapian::Query(Xapian::Query::OP_OR, v.begin(), v.end());
    }

    int m_last = (y1 < y2) ? 12 : m2 - 1;
    while (++m1 <= m_last) {
	sprintf(buf + 4 + bpoffs(), "%02d", m1);
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }
	
    if (y1 < y2) {
	while (++y1 < y2) {
	    sprintf(buf + bpoffs(), "%04d", y1);
	    bufprefix(buf, 'Y');
	    v.push_back(Xapian::Query(buf));
	}
	sprintf(buf + bpoffs(), "%04d", y2);
	bufprefix(buf, 'M');
	for (m1 = 1; m1 < m2; m1++) {
	    sprintf(buf + 4 + bpoffs(), "%02d", m1);
	    v.push_back(Xapian::Query(buf));
	}
    }
	
    sprintf(buf + 2 + bpoffs(), "%02d", m2);

    // Deal with any final partial month
    if (d2 < monthdays(m2, y2)) {
	bufprefix(buf, 'D');
    	for (d1 = 1 ; d1 <= d2; d1++) {
	    sprintf(buf + 6 + bpoffs(), "%02d", d1);
	    v.push_back(Xapian::Query(buf));
	}
    } else {
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }

    return Xapian::Query(Xapian::Query::OP_OR, v.begin(), v.end());
}


}

