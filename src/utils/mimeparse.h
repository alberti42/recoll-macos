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
/* @(#$Id: mimeparse.h,v 1.6 2006-09-05 08:04:36 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <map>

#include "base64.h"

/** A class to represent a MIME header value with parameters */
class MimeHeaderValue {
 public:
    std::string value;
    std::map<std::string, std::string> params;
};

/** 
 * Parse MIME Content-type and Content-disposition value
 *
 * @param in the input string should be like: value; pn1=pv1; pn2=pv2. 
 *   Example: text/plain; charset="iso-8859-1" 
 */
extern bool parseMimeHeaderValue(const std::string& in, MimeHeaderValue& psd);

/** Quoted printable decoding */
extern bool qp_decode(const std::string& in, std::string &out);

/** Decode an Internet mail header value encoded according to rfc2047 
 *
 * Example input:  =?iso-8859-1?Q?RE=A0=3A_Smoke_Tests?=
 * The input normally comes from parseMimeHeaderValue() output
 * and no comments or quoting are expected.
 * @param in input string, ascii with rfc2047 markup
 * @return out output string encoded in utf-8
 */
extern bool rfc2047_decode(const std::string& in, std::string &out);

#endif /* _MIME_H_INCLUDED_ */
