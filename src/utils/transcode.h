#ifndef _TRANSCODE_H_INCLUDED_
#define _TRANSCODE_H_INCLUDED_
/* @(#$Id: transcode.h,v 1.1 2004-12-15 09:43:48 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

extern bool transcode(const std::string &in, std::string &out, 
		      const std::string &icode,
		      const std::string &ocode);

#endif /* _TRANSCODE_H_INCLUDED_ */
