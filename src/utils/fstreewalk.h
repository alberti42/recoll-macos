#ifndef _FSTREEWALK_H_INCLUDED_
#define _FSTREEWALK_H_INCLUDED_
/* @(#$Id: fstreewalk.h,v 1.1 2004-12-10 18:13:13 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>


class FsTreeWalker {
 public:
    enum CbFlag {FtwRegular, FtwDirEnter, FtwDirReturn};
    enum Status {FtwOk=0, FtwError=1, FtwStop=2, 
		 FtwStatAll = FtwError|FtwStop};
    enum Options {FtwOptNone = 0, FtwNoRecurse = 1, FtwFollow = 2};

    typedef Status (*CbType)(void *cdata, 
			     const std::string &, const struct stat *, CbFlag);

    FsTreeWalker(Options opts = FtwOptNone);
    ~FsTreeWalker();
    Status walk(const std::string &dir, CbType fun, void *cdata);
    std::string getReason();
    int getErrCnt();
 private:
    class Internal;
    Internal *data;
};

#endif /* _FSTREEWALK_H_INCLUDED_ */
