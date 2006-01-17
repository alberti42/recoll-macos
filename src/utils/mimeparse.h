#ifndef _MIME_H_INCLUDED_
#define _MIME_H_INCLUDED_
/* @(#$Id: mimeparse.h,v 1.4 2006-01-17 10:08:51 dockes Exp $  (C) 2004 J.F.Dockes */

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
 * @param in input string, ascii with rfc2047 markup
 * @return out output string encoded in utf-8
 */
extern bool rfc2047_decode(const std::string& in, std::string &out);

#endif /* _MIME_H_INCLUDED_ */
