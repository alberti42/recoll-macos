#ifndef _TRANSCODE_H_INCLUDED_
#define _TRANSCODE_H_INCLUDED_
/* @(#$Id: transcode.h,v 1.2 2005-01-25 14:37:21 dockes Exp $  (C) 2004 J.F.Dockes */
/** 
 * A very minimal c++ized interface to iconv
 */
#include <string>

extern bool transcode(const std::string &in, std::string &out, 
		      const std::string &icode,
		      const std::string &ocode);

#endif /* _TRANSCODE_H_INCLUDED_ */
