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
#ifndef _UTF8ITER_H_INCLUDED_
#define _UTF8ITER_H_INCLUDED_
/* @(#$Id: utf8iter.h,v 1.7 2006-11-17 12:31:34 dockes Exp $  (C) 2004 J.F.Dockes */

/** 
 * A small helper class to iterate over utf8 strings. This is not an
 * STL iterator and this is not well designed, just convenient for
   some specific uses
 */
class Utf8Iter {
    unsigned int cl; // Char length at current position if known
    const string &s; // String we're working with
    string::size_type pos; // Current position in string
    unsigned int m_charpos; // Current character posiiton

    // Get character byte length at specified position
    inline int get_cl(string::size_type p) const {
	unsigned int z = (unsigned char)s[p];
	if (z <= 127) {
	    return 1;
	} else if (z>=192 && z <= 223) {
	    return 2;
	} else if (z >= 224 && z <= 239) {
	    return 3;
	} else if (z >= 240 && z <= 247) {
	    return 4;
	} else if (z >= 248 && z <= 251) {
	    return 5;
	} else if (z >= 252 && z <= 253) {
	    return 6;
	} 
	return -1;
    }
    // Check position and cl against string length
    bool poslok(string::size_type p, int l) const {
	return p != string::npos && l > 0 && p + l <= s.length();
    }
    // Update current char length in object state. Assumes pos is inside string
    inline int compute_cl() {
	cl = 0;
	cl = get_cl(pos);
	if (!poslok(pos, cl)) {
	    pos = s.length();
	    cl = 0;
	    return -1;
	}
	return 0;
    }
    // Compute value at given position
    inline unsigned int getvalueat(string::size_type p, int l) const {
	switch (l) {
	case 1: return (unsigned char)s[p];
	case 2: return ((unsigned char)s[p] - 192) * 64 + 
		(unsigned char)s[p+1] - 128 ;
	case 3: return ((unsigned char)s[p]-224)*4096 + 
		((unsigned char)s[p+1]-128)*64 + 
		(unsigned char)s[p+2]-128;
	case 4: return ((unsigned char)s[p]-240)*262144 + 
		((unsigned char)s[p+1]-128)*4096 + 
		((unsigned char)s[p+2]-128)*64 + 
		(unsigned char)s[p+3]-128;
	case 5: return ((unsigned char)s[p]-248)*16777216 + 
		((unsigned char)s[p+1]-128)*262144 + 
		((unsigned char)s[p+2]-128)*4096 + 
		((unsigned char)s[p+3]-128)*64 + 
		(unsigned char)s[p+4]-128;
	case 6: return  ((unsigned char)s[p]-252)*1073741824 + 
		((unsigned char)s[p+1]-128)*16777216 + 
		((unsigned char)s[p+2]-128)*262144 + 
		((unsigned char)s[p+3]-128)*4096 + 
		((unsigned char)s[p+4]-128)*64 + 
		(unsigned char)s[p+5]-128;
	default:
	    return (unsigned int)-1;
	}
    }
 public:
    Utf8Iter(const string &in) 
	: cl(0), s(in), pos(0), m_charpos(0) 
	{
	    // Ensure state is ok if appendchartostring is called at once
	    compute_cl();
	}

    void rewind() {
	cl=0; pos=0; m_charpos=0;
    }
    /** operator* returns the ucs4 value as a machine integer*/
    unsigned int operator*() {
	if (!cl && compute_cl() < 0)
	    return (unsigned int)-1;
	unsigned int val = getvalueat(pos, cl);
	if (val == (unsigned int)-1) {
	    pos = s.length();
	    cl = 0;
	}
	return val;
    }
    /** "Direct" access. Awfully inefficient as we skip from start or current
     * position at best. This can only be useful for a lookahead from the
     * current position */
    unsigned int operator[](unsigned int charpos) const {
	string::size_type mypos = 0;
	unsigned int mycp = 0;;
	if (charpos >= m_charpos) {
	    mypos = pos;
	    mycp = m_charpos;
	}
	while (mypos < s.length() && mycp != charpos) {
	    mypos += get_cl(mypos);
	    ++mycp;
	}
	if (mypos < s.length() && mycp == charpos) {
	    int l = get_cl(mypos);
	    if (poslok(mypos, l))
		return getvalueat(mypos, get_cl(mypos));
	}
	return (unsigned int)-1;
    }

    /** Set current position before next utf-8 character */
    string::size_type operator++(int) {
	if (!cl && compute_cl() < 0) {
	    return pos = string::npos;
	}
	pos += cl;
	m_charpos++;
	cl = 0;
	return pos;
    }
    /** This needs to be fast. No error checking. */
    void appendchartostring(string &out) {
	out.append(&s[pos], cl);
    }
    operator string() {
	if (!cl && compute_cl() < 0) {
	    return std::string("");
	}
	return s.substr(pos, cl);
    }
    bool eof() {
	// Note: we always ensure that pos == s.length() when setting bad to 
	// true
	return pos == s.length();
    }
    bool error() {
	return compute_cl() < 0;
    }
    string::size_type getBpos() const {
	return pos;
    }
    string::size_type getCpos() const {
	return m_charpos;
    }
};


#endif /* _UTF8ITER_H_INCLUDED_ */
