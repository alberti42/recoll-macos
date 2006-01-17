#ifndef _FSTREEWALK_H_INCLUDED_
#define _FSTREEWALK_H_INCLUDED_
/* @(#$Id: fstreewalk.h,v 1.4 2006-01-17 09:31:10 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif

class FsTreeWalkerCB;

/**
 * Class implementing a unix directory recursive walk.
 *
 * A user-defined function object is called for every file or
 * directory. Patterns to be ignored can be set before starting the
 * walk. Options control whether we follow symlinks and whether we recurse
 * on subdirectories.
 */
class FsTreeWalker {
 public:
    enum CbFlag {FtwRegular, FtwDirEnter, FtwDirReturn};
    enum Status {FtwOk=0, FtwError=1, FtwStop=2, 
		 FtwStatAll = FtwError|FtwStop};
    enum Options {FtwOptNone = 0, FtwNoRecurse = 1, FtwFollow = 2};

    FsTreeWalker(Options opts = FtwOptNone);
    ~FsTreeWalker();
    /** 
     * Begin file system walk.
     * @param dir is not checked against the ignored patterns (this is 
     *     a feature and must not change.
     * @param cb the function object that will be called back for every 
     *    file-system object (called both at entry and exit for directories).
     */
    Status walk(const string &dir, FsTreeWalkerCB& cb);
    /** Get explanation for error */
    string getReason();
    int getErrCnt();
    /**
     * Add a pattern to the list of things (file or dir) to be ignored
     * (ie: #* , *~)
     */
    bool addSkippedName(const string &pattern); 
    /** Set the ignored patterns list */
    bool setSkippedNames(const list<string> &patlist);
    /** Clear the ignored patterns list */
    void clearSkippedNames();

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
