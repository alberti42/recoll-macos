#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.4 2008-11-14 15:49:03 dockes Exp $  (C) 2005 J.F.Dockes */
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
    Rcl::Db   *m_rcldb;
    std::string m_dbdir;
    std::string m_reason;
    bool maybeOpenDb(string &reason);
    void outputError(const QString& errmsg);
};

#endif // _RECOLL_H
