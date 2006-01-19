#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.3 2006-01-19 14:57:59 dockes Exp $  (C) 2005 J.F.Dockes */
#define _RECOLL_H

#include <string>

#include <kio/global.h>
#include <kio/slavebase.h>

class RecollProtocol : public KIO::SlaveBase {
 public:
    RecollProtocol( const QCString &pool, const QCString &app );
    virtual ~RecollProtocol();
    virtual void get( const KURL & url );

 private:
    bool m_initok;
    RclConfig *m_rclconfig;
    Rcl::Db *m_rcldb;
    DocSequence *m_docsource;
    std::string m_dbdir;
    std::string m_reason;
    bool maybeOpenDb(string &reason);
    void outputError(const QString& errmsg);
};

#endif // _RECOLL_H
