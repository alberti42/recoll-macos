#ifndef _RCLINIT_H_INCLUDED_
#define _RCLINIT_H_INCLUDED_
/* @(#$Id: rclinit.h,v 1.2 2005-11-05 14:40:50 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

class RclConfig;

extern RclConfig *recollinit(void (*cleanup)(void), void (*sigcleanup)(int), 
			     std::string &reason);

#endif /* _RCLINIT_H_INCLUDED_ */
