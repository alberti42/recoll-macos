#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.3 2004-12-17 13:01:01 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

struct stat;

namespace Rcl {

/**
 * Holder for document attributes and data
 */
class Doc {
 public:
    string mimetype;
    string mtime;       // Modification time as decimal ascii
    string origcharset;
    string title;
    string text;
    string keywords;
    string abstract;
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
