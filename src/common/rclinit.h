#ifndef _RCLINIT_H_INCLUDED_
#define _RCLINIT_H_INCLUDED_
/* @(#$Id: rclinit.h,v 1.1 2005-04-05 09:35:35 dockes Exp $  (C) 2004 J.F.Dockes */

class RclConfig;

extern RclConfig *recollinit(void (*cleanup)(void), void (*sigcleanup)(int));

#endif /* _RCLINIT_H_INCLUDED_ */
