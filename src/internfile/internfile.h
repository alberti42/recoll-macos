#ifndef _INTERNFILE_H_INCLUDED_
#define _INTERNFILE_H_INCLUDED_
/* @(#$Id: internfile.h,v 1.3 2005-03-25 09:40:27 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

class MimeHandler;

/// Turn external file into internal representation, according to mime type etc
class FileInterner {
    string fn;
    RclConfig *config;
    const string &tdir;
    MimeHandler *handler;
    string tfile;
    string mime;

    void tmpcleanup();

 public:
    FileInterner(const std::string &f, RclConfig *cnf, const string& td);
    ~FileInterner();

    enum Status {FIError, FIDone, FIAgain};
    Status internfile(Rcl::Doc& doc, string &ipath);
};

#endif /* _INTERNFILE_H_INCLUDED_ */
