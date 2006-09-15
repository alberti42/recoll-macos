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
#ifndef _MIME_H_INCLUDED_
#define _MIME_H_INCLUDED_
/* @(#$Id: mimeparse.h,v 1.8 2006-09-15 16:50:44 dockes Exp $  (C) 2004 J.F.Dockes */

#include <time.h>

#include <string>
#include <map>

#include "base64.h"

#ifndef NO_NAMESPACES
using std::string;
#endif

/** A class to represent a MIME header value with parameters */
class MimeHeaderValue {
 public:
    string value;
    std::map<string, string> params;
};

/** 
 * Parse MIME Content-type and Content-disposition value
 *
 * @param in the input string should be like: value; pn1=pv1; pn2=pv2. 
 *   Example: text/plain; charset="iso-8859-1" 
 */
extern bool parseMimeHeaderValue(const string& in, MimeHeaderValue& psd);

/** Quoted printable decoding. Doubles up as rfc2231 decoder, hence the esc */
extern bool qp_decode(const string& in, string &out, 
		      char esc = '=');

/** Decode an Internet mail field value encoded according to rfc2047 
 *
 * Example input:  Some words =?iso-8859-1?Q?RE=A0=3A_Smoke_Tests?= more input
 * 
 * Note that MIME parameter values are explicitely NOT to be encoded with
 * this encoding which is only for headers like Subject:, To:. But it
 * is sometimes used anyway...
 * 
 * @param in input string, ascii with rfc2047 markup
 * @return out output string encoded in utf-8
 */
extern bool rfc2047_decode(const string& in, string &out);


/** Decode RFC2822 date to unix time (gmt secs from 1970
 *
 * @param dt date string (the part after Date: )
 * @return unix time
 */
time_t rfc2822DateToUxTime(const string& dt);

#endif /* _MIME_H_INCLUDED_ */
