#ifndef _UTF8ITER_H_INCLUDED_
#define _UTF8ITER_H_INCLUDED_
/* @(#$Id: utf8iter.h,v 1.1 2005-02-10 19:52:50 dockes Exp $  (C) 2004 J.F.Dockes */

/** 
 * A small helper class to iterate over utf8 strings. This is not an
 * STL iterator and this is not well designed, just convenient for
   some specific uses
 */
class Utf8Iter {
    unsigned int cl;
    const string &s;
    string::size_type pos;
    bool bad;
    int compute_cl() {
	cl = 0;
	if (bad)
	    return -1;
	unsigned int z = (unsigned char)s[pos];
	if (z <= 127) {
	    cl = 1;
	} else if (z>=192 && z <= 223) {
	    cl = 2;
	} else if (z >= 224 && z <= 239) {
	    cl = 3;
	} else if (z >= 240 && z <= 247) {
	    cl = 4;
	} else if (z >= 248 && z <= 251) {
	    cl = 5;
	} else if (z >= 252 && z <= 253) {
	    cl = 6;
	} 
	if (!cl || s.length() - pos < cl) {
	    bad = true;
	    cl = 0;
	    return -1;
	}
	return 0;
    }
 public:
    Utf8Iter(const string &in) : cl(0), s(in), pos(0), bad(false) {}

    /** operator* returns the ucs4 value as a machine integer*/
    unsigned int operator*() {
	if (!cl && compute_cl() < 0)
	    return (unsigned int)-1;
	switch (cl) {
	case 1: return (unsigned char)s[pos];
	case 2: return ((unsigned char)s[pos] - 192) * 64 + (unsigned char)s[pos+1] - 128 ;
	case 3: return ((unsigned char)s[pos]-224)*4096 + ((unsigned char)s[pos+1]-128)*64 + (unsigned char)s[pos+2]-128;
	case 4: return ((unsigned char)s[pos]-240)*262144 + ((unsigned char)s[pos+1]-128)*4096 + 
		((unsigned char)s[pos+2]-128)*64 + (unsigned char)s[pos+3]-128;
	case 5: return ((unsigned char)s[pos]-248)*16777216 + ((unsigned char)s[pos+1]-128)*262144 + 
		((unsigned char)s[pos+2]-128)*4096 + ((unsigned char)s[pos+3]-128)*64 + (unsigned char)s[pos+4]-128;
	case 6: return  ((unsigned char)s[pos]-252)*1073741824 + ((unsigned char)s[pos+1]-128)*16777216 + 
		((unsigned char)s[pos+2]-128)*262144 + ((unsigned char)s[pos+3]-128)*4096 + 
		((unsigned char)s[pos+4]-128)*64 + (unsigned char)s[pos+5]-128;
	default:
	    bad = true;
	    cl = 0;
	    return (unsigned int)-1;
	}
    }

    string::size_type operator++(int) {
	if (bad || (!cl && compute_cl() < 0)) {
	    return string::npos;
	}
	pos += cl;
	cl = 0;
	return pos;
    }

    bool appendchartostring(string &out) {
	if (bad || (!cl && compute_cl() < 0)) {
	    return false;
	}
	out += s.substr(pos, cl);
	return true;
    }
    bool eof() {
	return bad || pos == s.length();
    }
    bool error() {
	return bad;
    }
};


#endif /* _UTF8ITER_H_INCLUDED_ */
