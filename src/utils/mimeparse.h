#ifndef _MIME_H_INCLUDED_
#define _MIME_H_INCLUDED_
/* @(#$Id: mimeparse.h,v 1.1 2005-01-26 11:45:55 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <map>

// Simple (no quoting) mime value parser. Input: "value; pn1=pv1; pn2=pv2
class MimeHeaderValue {
 public:
    std::string value;
    std::map<std::string, std::string> params;
};
extern MimeHeaderValue parseMimeHeaderValue(const std::string &in);


#endif /* _MIME_H_INCLUDED_ */
