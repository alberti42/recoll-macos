/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _FSTREEWALK_H_INCLUDED_
#define _FSTREEWALK_H_INCLUDED_
/* @(#$Id: fstreewalk.h,v 1.8 2007-08-28 08:08:39 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif

class FsTreeWalkerCB;
struct stat;

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

    /** Same for skipped paths: this are paths, not names, under which we
	do not descend (ie: /home/me/.recoll) */
    bool addSkippedPath(const string &path); 
    /** Set the ignored paths list */
    bool setSkippedPaths(const list<string> &pathlist);

    bool inSkippedPaths(const string& path);
    bool inSkippedNames(const string& name);
 private:
    Status iwalk(const string &dir, struct stat *stp, FsTreeWalkerCB& cb);
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
