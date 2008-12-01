#ifndef _RECOLL_H
/* @(#$Id: kio_recoll.h,v 1.11 2008-12-01 18:42:52 dockes Exp $  (C) 2005 J.F.Dockes */
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
#include <qstring.h>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <kdeversion.h>

#include "rclconfig.h"
#include "rcldb.h"
#include "reslistpager.h"
#include "docseq.h"
#include "refcntr.h"

class RecollProtocol;

/** Specialize the recoll html pager for the kind of links we use etc. */
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

/**
 * A KIO slave to execute and display Recoll searches.
 *
 * Things are made a little complicated because KIO slaves can't hope
 * that their internal state will remain consistent with their user
 * application state: slaves die, are restarted, reused, at random
 * between requests. 
 * In our case, this means that any request has to be processed
 * without reference to the last operation performed. Ie, if the
 * search parameters are not those from the last request, the search
 * must be restarted anew. This happens for example with different
 * searches in 2 konqueror screens: typically only one kio_slave will
 * be used.
 * The fact that we check if the search is the same as the last one,
 * to avoid restarting is an optimization, not the base mechanism
 * (contrary to what was initially assumed, and may have left a few
 * crumbs around).
 *
 * We have two modes of operation, one based on html forms and result
 * pages, which can potentially be developped to the full Recoll
 * functionality, and one based on a directory listing model, which
 * will always be more limited, but may be useful in some cases to
 * allow easy copying of files etc. Which one is in use is decided by
 * the form of the URL. 
 */
class RecollProtocol : public KIO::SlaveBase {
 public:
    RecollProtocol(const QByteArray &pool, const QByteArray &app );
    virtual ~RecollProtocol();
    virtual void mimetype(const KUrl& url);
    virtual void get(const KUrl& url);
    // The directory mode is not available with KDE 4.0, I could find
    // no way to avoid crashing kdirmodel
#if KDE_IS_VERSION(4,1,0)
    virtual void stat(const KUrl & url);
    virtual void listDir(const KUrl& url);
#endif

    static RclConfig  *o_rclconfig;

    friend class RecollKioPager;

 private:
    bool maybeOpenDb(string& reason);
    bool URLToQuery(const KUrl &url, QString& q, QString& opt, int *page=0);
    bool doSearch(const QString& q, const QString& opt = "l");

    void welcomePage();
    void queryDetails();
    string makeQueryUrl(int page);
    void htmlDoSearch(const QString& q, const QString& opt, int page);

    bool isRecollResult(const KUrl &url, int *num);

    bool        m_initok;
    Rcl::Db    *m_rcldb;
    string      m_reason;

    // Search state: because of how the KIO slaves are used / reused,
    // we can't be sure that the next request will be for the same
    // search, and we need to check and restart one if the data
    // changes. This is very wasteful but hopefully won't happen too
    // much in actual use. One possible workaround for some scenarios
    // (one slave several konqueror windows) would be to have a small
    // cache of recent searches kept open.
    RecollKioPager m_pager;
    RefCntr<DocSequence> m_source;
    QString     m_srchStr;
    QString     m_opt;
};

extern "C" {int kdemain(int, char**);}

#endif // _RECOLL_H
