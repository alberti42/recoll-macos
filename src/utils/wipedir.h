#ifndef _FILEUT_H_INCLUDED_
#define _FILEUT_H_INCLUDED_
/* @(#$Id: wipedir.h,v 1.1 2005-02-09 12:07:30 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/**
 *  Remove all files inside directory (not recursive). 
 * @return  0 if ok, count of remaining entries (ie: subdirs), or -1 for error
 */
int wipedir(const std::string& dirname);

#endif /* _FILEUT_H_INCLUDED_ */
