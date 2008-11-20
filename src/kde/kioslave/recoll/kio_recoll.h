#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.6 2008-11-20 13:10:23 dockes Exp $  (C) 2005 J.F.Dockes */
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

#include "reslistpager.h"

class RecollProtocol;

class RecollKioPager : public ResListPager {
public:
    RecollKioPager() : m_parent(0) {}
    void setParent(RecollProtocol *proto) {m_parent = proto;}

    virtual bool append(const string& data);
    virtual string detailsLink();
    virtual const string &parFormat();
    virtual string nextUrl();
    virtual string prevUrl();
    virtual string pageTop();
private:
    RecollProtocol *m_parent;
};

class RecollProtocol : public KIO::SlaveBase {
 public:
    RecollProtocol(const QByteArray &pool, const QByteArray &app );
    virtual ~RecollProtocol();
    virtual void mimetype(const KUrl & url );
    virtual void get(const KUrl & url );

 private:
    bool maybeOpenDb(string &reason);
    void outputError(const QString& errmsg);
    void doSearch(const QString& q, char opt = 'l');
    void welcomePage();

    bool        m_initok;
    RclConfig  *m_rclconfig;
    Rcl::Db    *m_rcldb;
    std::string m_dbdir;
    std::string m_reason;
    RecollKioPager m_pager;

};
extern "C" {int kdemain(int, char**);}

#endif // _RECOLL_H
