#ifndef _RECOLL_H
#define _RECOLL_H

#include <string>

#include <kio/global.h>
#include <kio/slavebase.h>

class RecollProtocol : public KIO::SlaveBase
{
public:
    RecollProtocol( const QCString &pool, const QCString &app );
    virtual ~RecollProtocol();
    virtual void mimetype(const KURL& url);
    virtual void get( const KURL & url );

    //    virtual void listDir( const KURL & url );
    //    virtual void stat( const KURL & url );
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
#endif
