#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.2 2004-12-15 15:00:36 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

struct stat;

namespace Rcl {

/**
 * Holder for document attributes and data
 */
class Doc {
 public:
    string origcharset;
    string title;
    string abstract;
    string keywords;
    string text;
};

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
    bool add(const string &filename, const Doc &doc);
    bool needUpdate(const string &filename, const struct stat *stp);
};


}

#endif /* _DB_H_INCLUDED_ */
