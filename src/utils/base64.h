#ifndef _BASE64_H_INCLUDED_
#define _BASE64_H_INCLUDED_
/* @(#$Id: base64.h,v 1.1 2005-11-06 11:16:53 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

void base64_encode(const std::string &in, std::string &out);
bool base64_decode(const std::string& in, std::string& out);

#endif /* _BASE64_H_INCLUDED_ */
