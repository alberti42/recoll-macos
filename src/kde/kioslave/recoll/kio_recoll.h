#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.5 2008-11-17 14:51:38 dockes Exp $  (C) 2005 J.F.Dockes */
#define _RECOLL_H

#include <string>
using std::string;

#include <qglobal.h>

#if (QT_VERSION < 0x040000)
#include <qstring.h>
#else
#include <QString>
#endif

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

class RecollProtocol : public KIO::SlaveBase {
 public:
    RecollProtocol(const QByteArray &pool, const QByteArray &app );
    virtual ~RecollProtocol();
    virtual void mimetype(const KUrl & url );
    virtual void get(const KUrl & url );

 private:
    bool m_initok;
    RclConfig *m_rclconfig;
    Rcl::Db   *m_rcldb;
    std::string m_dbdir;
    std::string m_reason;
    bool maybeOpenDb(string &reason);
    void outputError(const QString& errmsg);
};
extern "C" {int kdemain(int, char**);}

#endif // _RECOLL_H
