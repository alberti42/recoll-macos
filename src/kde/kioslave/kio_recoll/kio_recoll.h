#ifndef _RECOLL_H
#define _RECOLL_H
/* Copyright (C) 2005 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string>
#include <memory>

#include <QString>
#include <QUrl>

#include <kio_version.h>
// Apparently this nonsense happened in 5.102 (0x66). 
#define KIO_WORKER_SWITCH_VERSION 0x056600
#if KIO_VERSION < KIO_WORKER_SWITCH_VERSION
#include <kio/slavebase.h>
#define WBASE SlaveBase
#define WRESULT void
#else
#include <kio/workerbase.h>
#define WBASE WorkerBase
#define WRESULT KIO::WorkerResult
#endif

#include "reslistpager.h"

class RecollProtocol;
class RclConfig;
class DocSeq;
namespace Rcl {
class Db;
}

/** Specialize the recoll html pager for the kind of links we use etc. */
class RecollKioPager : public ResListPager {
public:
    RecollKioPager(RclConfig *cnf)
        : ResListPager(cnf), m_parent(nullptr) {}
    void setParent(RecollProtocol *proto) {
        m_parent = proto;
    }

    virtual bool append(const std::string& data) override;
    virtual bool append(const std::string& data, int, const Rcl::Doc&) override {
        return append(data);
    }
    virtual std::string detailsLink() override;
    virtual const std::string& parFormat() override;
    virtual std::string nextUrl() override;
    virtual std::string prevUrl() override;
    virtual std::string pageTop() override;

private:
    RecollProtocol *m_parent;
};

class QueryDesc {
public:
    QueryDesc() : opt(QString::fromUtf8("l")), page(0), isDetReq(false) {}
    QString query;
    QString opt;
    int page;
    bool isDetReq;
    bool sameQuery(const QueryDesc& o) const {
        return !opt.compare(o.opt) && !query.compare(o.query);
    }
};

// Our virtual tree is a bit complicated. We need a class to analyse an URL
// and tell what we should do with it
class UrlIngester {
public:
    UrlIngester(RecollProtocol *p, const QUrl& url);
    enum RootEntryType {UIRET_NONE, UIRET_ROOT, UIRET_HELP, UIRET_SEARCH};
    bool isRootEntry(RootEntryType *tp) {
        if (m_type != UIMT_ROOTENTRY) {
            return false;
        }
        *tp = m_retType;
        return true;
    }
    bool isQuery(QueryDesc *q) {
        if (m_type != UIMT_QUERY) {
            return false;
        }
        *q = m_query;
        return true;
    }
    bool isResult(QueryDesc *q, int *num) {
        if (m_type != UIMT_QUERYRESULT) {
            return false;
        }
        *q = m_query;
        *num = m_resnum;
        return true;
    }
    bool isPreview(QueryDesc *q, int *num) {
        if (m_type != UIMT_PREVIEW) {
            return false;
        }
        *q = m_query;
        *num = m_resnum;
        return true;
    }
    bool endSlashQuery() {
        return m_slashend;
    }
    bool alwaysDir() {
        return m_alwaysdir;
    }

private:
    RecollProtocol *m_parent;
    QueryDesc       m_query;
    bool            m_slashend;
    bool            m_alwaysdir;
    RootEntryType   m_retType;
    int             m_resnum;
    enum MyType {UIMT_NONE, UIMT_ROOTENTRY, UIMT_QUERY, UIMT_QUERYRESULT,
                 UIMT_PREVIEW
                };
    MyType           m_type;
};

/**
 * A KIO worker to execute and display Recoll searches.
 *
 * Things are made a little complicated because KIO workers can't hope
 * that their internal state will remain consistent with their user
 * application state: workers die, are restarted, reused, at random
 * between requests.
 * In our case, this means that any request has to be processed
 * without reference to the last operation performed. Ie, if the
 * search parameters are not those from the last request, the search
 * must be restarted anew. This happens for example with different
 * searches in 2 konqueror screens: typically only one kio_worker will
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
class RecollProtocol : public KIO::WBASE
{
public:
    RecollProtocol(const QByteArray& pool, const QByteArray& app);
    virtual ~RecollProtocol();
    virtual WRESULT mimetype(const QUrl& url) override;
    virtual WRESULT get(const QUrl& url) override;
    // The directory mode is not available with KDE 4.0, I could find
    // no way to avoid crashing kdirmodel
    virtual WRESULT stat(const QUrl& url) override;
    virtual WRESULT listDir(const QUrl& url) override;

    static RclConfig  *o_rclconfig;

    friend class RecollKioPager;
    friend class UrlIngester;

private:
    bool maybeOpenDb(std::string& reason);
    bool URLToQuery(const QUrl& url, QString& q, QString& opt, int *page = nullptr);
    bool doSearch(const QueryDesc& qd);

    void searchPage();
    void queryDetails();
    std::string makeQueryUrl(int page, bool isdet = false);
    bool syncSearch(const QueryDesc& qd);
    void htmlDoSearch(const QueryDesc& qd);
    void showPreview(const Rcl::Doc& doc);
    bool isRecollResult(const QUrl& url, int *num, QString* q);

    bool        m_initok;
    std::shared_ptr<Rcl::Db> m_rcldb;
    std::string      m_reason;
    bool        m_alwaysdir;
    // english by default else env[RECOLL_KIO_STEMLANG]
    std::string      m_stemlang;

    // Search state: because of how the KIO workers are used / reused,
    // we can't be sure that the next request will be for the same
    // search, and we need to check and restart one if the data
    // changes. This is very wasteful but hopefully won't happen too
    // much in actual use. One possible workaround for some scenarios
    // (one worker several konqueror windows) would be to have a small
    // cache of recent searches kept open.
    std::unique_ptr<RecollKioPager> m_pager;
    std::shared_ptr<DocSequence> m_source;
    // Note: page here is not used, current page always comes from m_pager.
    QueryDesc      m_query;
    // Display results from embedded documents. The KIO does not know what to do
    // with them. This can be disabled from recoll.conf with kio_showsubdocs = 0
    bool m_showSubdocs{true};
};

extern "C" {
    __attribute__((visibility("default"))) int
    kdemain(int argc, char **argv);
}


#endif // _RECOLL_H
