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
#ifndef _SMALLUT_H_INCLUDED_
#define _SMALLUT_H_INCLUDED_
/* @(#$Id: smallut.h,v 1.33 2008-12-05 07:38:07 dockes Exp $  (C) 2004 J.F.Dockes */
#include <stdlib.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
using std::map;
using std::set;
#endif /* NO_NAMESPACES */

// Note these are all ascii routines
extern int stringicmp(const string& s1, const string& s2);
// For find_if etc.
struct StringIcmpPred {
    StringIcmpPred(const string& s1) 
        : m_s1(s1) 
    {}
    bool operator()(const string& s2) {
        return stringicmp(m_s1, s2) == 0;
    }
    const string& m_s1;
};

extern int stringlowercmp(const string& alreadylower, const string& s2);
extern int stringuppercmp(const string& alreadyupper, const string& s2); 
extern void stringtolower(string& io);
extern string stringtolower(const string& io);
// Is one string the end part of the other ?
extern int stringisuffcmp(const string& s1, const string& s2);

// Compare charset names, removing the more common spelling variations
extern bool samecharset(const string &cs1, const string &cs2);

/**
 * Parse input string into list of strings. 
 *
 * Token delimiter is " \t\n" except inside dquotes. dquote inside
 * dquotes can be escaped with \ etc...
 * Input is handled a byte at a time, things will work as long as space tab etc.
 * have the ascii values and can't appear as part of a multibyte char. utf-8 ok
 * but so are the iso-8859-x and surely others. addseps do have to be 
 * single-bytes
 */
extern bool stringToStrings(const string& s, list<string> &tokens, 
                            const string& addseps = "");
extern bool stringToStrings(const string& s, vector<string> &tokens, 
                            const string& addseps = "");
extern bool stringToStrings(const string& s, set<string> &tokens, 
                            const string& addseps = "");

/**
 * Inverse operation:
 */
extern void stringsToString(const list<string> &tokens, string &s);
extern void stringsToString(const vector<string> &tokens, string &s);
extern void stringsToString(const set<string> &tokens, string &s);

/**
 * Split input string. No handling of quoting
 */
extern void stringToTokens(const string &s, list<string> &tokens, 
			   const string &delims = " \t", bool skipinit=true);

/** Convert string to boolean */
extern bool stringToBool(const string &s);

/** Remove instances of characters belonging to set (default {space,
    tab}) at beginning and end of input string */
extern void trimstring(string &s, const char *ws = " \t");

/** Escape things like < or & by turning them into entities */
extern string escapeHtml(const string &in);

/** Replace some chars with spaces (ie: newline chars). This is not utf8-aware
 *  so chars should only contain ascii */
extern string neutchars(const string &str, const string &chars);
extern void neutchars(const string &str, string& out, const string &chars);

/** Turn string into something that won't be expanded by a shell. In practise
 *  quote with double-quotes and escape $`\ */
extern string escapeShell(const string &str);

/** Truncate a string to a given maxlength, avoiding cutting off midword
 *  if reasonably possible. */
extern string truncate_to_word(const string &input, string::size_type maxlen);

/** Truncate in place in an utf8-legal way */
extern void utf8truncate(string &s, int maxlen);

/** Convert byte count into unit (KB/MB...) appropriate for display */
string displayableBytes(long size);

/** Break big string into lines */
string breakIntoLines(const string& in, unsigned int ll = 100, 
		      unsigned int maxlines= 50);
/** Small utility to substitute printf-like percents cmds in a string */
bool pcSubst(const string& in, string& out, map<char, string>& subs);
/** Substitute printf-like percents and also %(key) */
bool pcSubst(const string& in, string& out, map<string, string>& subs);

/** Compute times to help with perf issues */
class Chrono {
 public:
  Chrono();
  /** Reset origin */
  long restart();
  /** Snapshot current time */
  static void refnow();
  /** Get current elapsed since creation or restart
   *
   *  @param frozen give time since the last refnow call (this is to
   * allow for using one actual system call to get values from many
   * chrono objects, like when examining timeouts in a queue)
   */
  long millis(int frozen = 0);
  long ms() {return millis();}
  long micros(int frozen = 0);
  float secs(int frozen = 0);
 private:
  long	m_secs;
  long 	m_nsecs; 
};

/** Temp buffer with automatic deallocation */
struct TempBuf {
    TempBuf() 
        : m_buf(0)
    {}
    TempBuf(int n)
    {
        m_buf = (char *)malloc(n);
    }
    ~TempBuf()
    { 
        if (m_buf)
            free(m_buf);
    }
    char *setsize(int n) { return (m_buf = (char *)realloc(m_buf, n)); }
    char *buf() {return m_buf;}
    char *m_buf;
};

#ifndef deleteZ
#define deleteZ(X) {delete X;X = 0;}
#endif

#endif /* _SMALLUT_H_INCLUDED_ */
