#ifndef _FSTREEWALK_H_INCLUDED_
#define _FSTREEWALK_H_INCLUDED_
/* @(#$Id: fstreewalk.h,v 1.2 2005-02-10 15:21:12 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#ifndef NO_NAMESPACES
using std::string;
#endif

class FsTreeWalkerCB;

class FsTreeWalker {
 public:
    enum CbFlag {FtwRegular, FtwDirEnter, FtwDirReturn};
    enum Status {FtwOk=0, FtwError=1, FtwStop=2, 
		 FtwStatAll = FtwError|FtwStop};
    enum Options {FtwOptNone = 0, FtwNoRecurse = 1, FtwFollow = 2};

    FsTreeWalker(Options opts = FtwOptNone);
    ~FsTreeWalker();
    Status walk(const std::string &dir, FsTreeWalkerCB& cb);
    std::string getReason();
    int getErrCnt();
 private:
    class Internal;
    Internal *data;
};

class FsTreeWalkerCB {
 public:
    virtual ~FsTreeWalkerCB() {}
    virtual FsTreeWalker::Status 
	processone(const string &, const struct stat *, FsTreeWalker::CbFlag) 
	= 0;
};
#endif /* _FSTREEWALK_H_INCLUDED_ */
