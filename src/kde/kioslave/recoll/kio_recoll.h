#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.8 2008-11-27 17:48:43 dockes Exp $  (C) 2005 J.F.Dockes */
#define _RECOLL_H
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

#include "rclconfig.h"
#include "rcldb.h"
#include "reslistpager.h"
#include "docseq.h"
#include "refcntr.h"

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
    virtual void stat(const KUrl & url);
    virtual void listDir(const KUrl& url);

    static RclConfig  *o_rclconfig;

 private:
    bool maybeOpenDb(string &reason);
    void outputError(const QString& errmsg);
    bool doSearch(const QString& q, char opt = 'l');
    void welcomePage();
    void queryDetails();
    void htmlDoSearch(const QString& q, char opt);
    bool URLToQuery(const KUrl &url, QString& q, QString& opt);
    void createRootEntry(KIO::UDSEntry& entry);
    void createGoHomeEntry(KIO::UDSEntry& entry);

    bool        m_initok;
    Rcl::Db    *m_rcldb;
    std::string m_reason;
    RecollKioPager m_pager;
    RefCntr<DocSequence> m_source;
};
extern "C" {int kdemain(int, char**);}

#endif // _RECOLL_H
