#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.1 2004-12-14 17:50:28 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

namespace Rcl {

/**
 * Wrapper class for the native database.
 */
class Db {
    void *pdata;
 public:
    Db();
    ~Db();
    enum OpenMode {DbRO, DbUpd, DbTrunc};
    bool open(const std::string &dbdir, OpenMode mode);
    bool close();
};

class Doc {
 public:
    string title;
    string abstract;
    string keywords;
    string text;
};

}

#endif /* _DB_H_INCLUDED_ */
