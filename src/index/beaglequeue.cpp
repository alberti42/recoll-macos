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
#ifndef lint
static char rcsid[] = "@(#$Id: $ (C) 2005 J.F.Dockes";
#endif
#include "autoconfig.h"
#include "pathut.h"
#include "debuglog.h"
#include "fstreewalk.h"
#include "beaglequeue.h"
#include "smallut.h"
#include "fileudi.h"
#include "internfile.h"
#include "wipedir.h"
#include "circache.h"

#include <vector>
#include <fstream>
using namespace std;

#include <sys/stat.h>

const string keybght("beagleHitType");

#define LL 2048

class BeagleDotFile {
public:
    BeagleDotFile(RclConfig *conf, const string& fn)
        : m_conf(conf), m_fn(fn)
    {

    }

    bool readLine(string& line)
    {
        char cline[LL]; 
        cline[0] = 0;
        m_input.getline(cline, LL-1);
        if (!m_input.good()) {
            if (m_input.bad()) {
                LOGERR(("beagleDotFileRead: input.bad()\n"));
            }
            return false;
        }
        int ll = strlen(cline);
        while (ll > 0 && (cline[ll-1] == '\n' || cline[ll-1] == '\r')) {
            cline[ll-1] = 0;
            ll--;
        }
        line.assign(cline, ll);
        LOGDEB2(("BeagleDotFile:readLine: [%s]\n", line.c_str()));
        return true;
    }

    // Process a beagle dot file and set interesting stuff in the doc
    bool toDoc(Rcl::Doc& doc)
    {
        string line;

	m_input.open(m_fn.c_str(), ios::in);
        if (!m_input.good()) {
            LOGERR(("BeagleDotFile: open failed for [%s]\n", m_fn.c_str()));
            return false;
        }

        // Read the 3 first lines: 
        // - url
        // - hit type: we only know about Bookmark and WebHistory for now
        // - content-type.
        if (!readLine(line))
            return false;
        doc.url = line;
        if (!readLine(line))
            return false;
        doc.meta[keybght] = line;
        if (!readLine(line))
            return false;
        doc.mimetype = line;

        if (doc.mimetype.empty() && 
            !stringlowercmp("bookmark", doc.meta[keybght]))
            doc.mimetype = "text/plain";

        string confstr;
        string ss(" ");
        // Read the rest: fields and keywords
        for (;;) {
            if (!readLine(line)) {
                // Eof hopefully
                break;
            }
            if (line.find("t:") != 0)
                continue;
            line = line.substr(2);
            confstr += line + "\n";
        }

        ConfSimple fields(confstr, 1);
        list<string> names = fields.getNames("");
        for (list<string>::iterator it = names.begin();
             it != names.end(); it++) {
            string value;
            fields.get(*it, value, "");
            if (!value.compare("undefined") || !value.compare("null"))
                continue;
            string caname = m_conf->fieldCanon(*it);
            doc.meta[caname].append(ss + value);
        }
        return true;
    }    

    RclConfig *m_conf;
    string m_fn;
    ifstream m_input;
};

const string badtmpdirname = "/no/such/dir/really/can/exist";
BeagleQueueIndexer::BeagleQueueIndexer(RclConfig *cnf)
    : m_config(cnf), m_db(cnf)
{
    if (!m_config->getConfParam("beaglequeuedir", m_queuedir))
        m_queuedir = path_tildexpand("~/.beagle/ToIndex");
    if (m_tmpdir.empty() || access(m_tmpdir.c_str(), 0) < 0) {
	string reason;
        if (!maketmpdir(m_tmpdir, reason)) {
	    LOGERR(("DbIndexer: cannot create temporary directory: %s\n",
		    reason.c_str()));
            m_tmpdir = badtmpdirname;
	}
    }
    Rcl::Db::OpenMode mode = Rcl::Db::DbUpd;
    if (!m_db.open(mode)) {
	LOGERR(("BeagleQueueIndexer: error opening database %s\n", 
                m_config->getDbDir().c_str()));
	return;
    }
}

BeagleQueueIndexer::~BeagleQueueIndexer()
{
    LOGDEB(("BeagleQueueIndexer::~\n"));
    if (m_tmpdir.length() && m_tmpdir.compare(badtmpdirname)) {
	wipedir(m_tmpdir);
	if (rmdir(m_tmpdir.c_str()) < 0) {
	    LOGERR(("BeagleQueueIndexer::~: cannot clear temp dir %s\n",
		    m_tmpdir.c_str()));
	}
    }
    m_db.close();
}

bool BeagleQueueIndexer::processqueue()
{
    LOGDEB(("BeagleQueueIndexer::processqueue: dir: [%s]\n", 
            m_queuedir.c_str()));

    FsTreeWalker walker(FsTreeWalker::FtwNoRecurse);
    walker.addSkippedName(".*");
    FsTreeWalker::Status status =walker.walk(m_queuedir, *this);
    LOGDEB(("BeagleQueueIndexer::processqueue: done: status %d\n", status));
    return true;
}

FsTreeWalker::Status 
BeagleQueueIndexer::processone(const string &path,
                               const struct stat *stp,
                               FsTreeWalker::CbFlag flg)
{
    if (flg != FsTreeWalker::FtwRegular) 
        return FsTreeWalker::FtwOk;

    string dotpath = path_cat(path_getfather(path), 
                              string(".") + path_getsimple(path));
    LOGDEB(("BeagleQueueIndexer: prc1: [%s]\n", path.c_str()));
    BeagleDotFile dotfile(m_config, dotpath);
    Rcl::Doc dotdoc;
    string udi, udipath;
    if (!dotfile.toDoc(dotdoc))
        goto out;
    //dotdoc.dump(1);

    // Have to use the hit type for the udi, because the same url can exist
    // as a bookmark or a page.
    udipath = path_cat(dotdoc.meta[keybght], url_gpath(dotdoc.url));
    make_udi(udipath, "", udi);

    LOGDEB(("BeagleQueueIndexer: prc1: udi [%s]\n", udi.c_str()));
    char ascdate[20];
    sprintf(ascdate, "%ld", long(stp->st_mtime));

    // We only process bookmarks or text/html and text/plain files.
    if (!stringlowercmp("bookmark", dotdoc.meta[keybght])) {

    } else if (stringlowercmp("webhistory", dotdoc.meta[keybght]) ||
               (dotdoc.mimetype.compare("text/html") &&
                dotdoc.mimetype.compare("text/plain"))) {
        LOGDEB(("BeagleQueueIndexer: skipping: hittype %s mimetype %s\n",
                dotdoc.meta[keybght].c_str(), dotdoc.mimetype.c_str()));
        goto out;
    } else {
        Rcl::Doc doc;
        FileInterner interner(path, stp, m_config, m_tmpdir, 
                              FileInterner::FIF_doUseInputMimetype,
                              &dotdoc.mimetype);
        string ipath;
        FileInterner::Status fis = interner.internfile(doc, ipath);
        if (fis != FileInterner::FIDone) {
            LOGERR(("BeagleQueueIndexer: bad status from internfile\n"));
            goto out;
        }

        if (doc.fmtime.empty())
            doc.fmtime = ascdate;

        char cbuf[100]; 
        sprintf(cbuf, "%ld", (long)stp->st_size);
        doc.fbytes = cbuf;
        // Document signature for up to date checks: none. The file is
        // going to be deleted anyway. We always reindex what comes in
        // the queue.  It would probably be possible to extract some
        // http data to avoid this.
        doc.sig = "";
        doc.url = dotdoc.url;
        if (!m_db.addOrUpdate(udi, "", doc)) 
            return FsTreeWalker::FtwError;
    }
out:
//    unlink(path.c_str());
//    unlink(dotpath.c_str());
    return FsTreeWalker::FtwOk;
}
