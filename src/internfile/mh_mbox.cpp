/* Copyright (C) 2005-2019 J.F.Dockes
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
#include "autoconfig.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#include <cstring>
#include <map>
#include <mutex>

#include "cstr.h"
#include "mimehandler.h"
#include "log.h"
#include "readfile.h"
#include "mh_mbox.h"
#include "smallut.h"
#include "rclconfig.h"
#include "md5ut.h"
#include "conftree.h"
#include "pathut.h"

using namespace std;

#ifdef _WIN32
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

// Define maximum message size for safety. 100MB would seem reasonable
static const unsigned int max_mbox_member_size = 100 * 1024 * 1024;

// The mbox format uses lines beginning with 'From ' as separator.
// Mailers are supposed to quote any other lines beginning with 
// 'From ', turning it into '>From '. This should make it easy to detect
// message boundaries by matching a '^From ' regular expression
// Unfortunately this quoting is quite often incorrect in the real world.
//
// The rest of the format for the line is somewhat variable, but there will 
// be a 4 digit year somewhere... 
// The canonic format is the following, with a 24 characters date: 
//         From toto@tutu.com Sat Sep 30 16:44:06 2000
// This resulted into the pattern for versions up to 1.9.0: 
//         "^From .* [1-2][0-9][0-9][0-9]$"
//
// Some mailers add a time zone to the date, this is non-"standard", 
// but happens, like in: 
//    From toto@truc.com Sat Sep 30 16:44:06 2000 -0400 
//
// This is taken into account in the new regexp, which also matches more
// of the date format, to catch a few actual issues like
//     From http://www.itu.int/newsroom/press/releases/1998/NP-2.html:
// Note that this *should* have been quoted.
//
// http://www.qmail.org/man/man5/mbox.html seems to indicate that the
// fact that From_ is normally preceded by a blank line should not be
// used, but we do it anyway (for now).
// The same source indicates that arbitrary data can follow the date field
//
// A variety of pathologic From_ lines:
//   Bad date format:
//      From uucp Wed May 22 11:28 GMT 1996
//   Added timezone at the end (ok, part of the "any data" after the date)
//      From qian2@fas.harvard.edu Sat Sep 30 16:44:06 2000 -0400
//  Emacs VM botch ? Adds tz between hour and year
//      From dockes Wed Feb 23 10:31:20 +0100 2005
//      From dockes Fri Dec  1 20:36:39 +0100 2006
// The modified regexp gives the exact same results on the ietf mail archive
// and my own's.
// Update, 2008-08-29: some old? Thunderbird versions apparently use a date
// in "Date: " header format, like:   From - Mon, 8 May 2006 10:57:32
// This was added as an alternative format. By the way it also fools "mail" and
// emacs-vm, Recoll is not alone
// Update: 2009-11-27: word after From may be quoted string: From "john bull"
static const string frompat{
    "^From[ ]+([^ ]+|\"[^\"]+\")[ ]+"    // 'From (toto@tutu|"john bull") '
        "[[:alpha:]]{3}[ ]+[[:alpha:]]{3}[ ]+[0-3 ][0-9][ ]+" // Fri Oct 26
        "[0-2][0-9]:[0-5][0-9](:[0-5][0-9])?[ ]+"             // Time, seconds optional
        "([^ ]+[ ]+)?"                                        // Optional tz
        "[12][0-9][0-9][0-9]"            // Year, unanchored, more data may follow
        "|"      // Or standard mail Date: header format
        "^From[ ]+[^ ]+[ ]+"                                   // From toto@tutu
        "[[:alpha:]]{3},[ ]+[0-3]?[0-9][ ]+[[:alpha:]]{3}[ ]+" // Mon, 8 May
        "[12][0-9][0-9][0-9][ ]+"                              // Year
        "[0-2][0-9]:[0-5][0-9](:[0-5][0-9])?"                  // Time, secs optional
        };

// Extreme thunderbird brokiness. Will sometimes use From lines
// exactly like: From ^M (From followed by space and eol). We only
// test for this if QUIRKS_TBIRD is set
static const string miniTbirdFrom{"^From $"};

static SimpleRegexp fromregex(frompat, SimpleRegexp::SRE_NOSUB);
static SimpleRegexp minifromregex(miniTbirdFrom, SimpleRegexp::SRE_NOSUB);

// Automatic fp closing
class FpKeeper { 
public:
    FpKeeper(FILE **fpp)
        : m_fpp(fpp) {}
    ~FpKeeper() {
        if (m_fpp && *m_fpp) {
            fclose(*m_fpp);
            *m_fpp = 0;
        }
    }
private:
    FILE **m_fpp;
};

static std::mutex o_mcache_mutex;

/**
 * Handles a cache for message numbers to offset translations. Permits direct
 * accesses inside big folders instead of having to scan up to the right place
 *
 * Message offsets are saved to files stored under cfg(mboxcachedir), default
 * confdir/mboxcache. Mbox files smaller than cfg(mboxcacheminmbs) are not
 * cached.
 * Cache files are named as the md5 of the file UDI, which is kept in
 * the first block for possible collision detection. The 64 bits
 * offsets for all message "From_" lines follow. The format is purely
 * binary, values are not even byte-swapped to be proc-idependant.
 */

#define M_o_b1size 1024

class MboxCache {
public:
    MboxCache() { 
        // Can't access rclconfig here, we're a static object, would
        // have to make sure it's initialized.
    }

    ~MboxCache() {}

    int64_t get_offset(RclConfig *config, const string& udi, int msgnum) {
        LOGDEB0("MboxCache::get_offsets: udi [" << udi << "] msgnum "
                << msgnum << "\n");
        if (!ok(config)) {
            LOGDEB0("MboxCache::get_offsets: init failed\n");
            return -1;
        }
        std::unique_lock<std::mutex> locker(o_mcache_mutex);
        string fn = makefilename(udi);
        FILE *fp = 0;
        if ((fp = fopen(fn.c_str(), "rb")) == 0) {
            LOGSYSERR("MboxCache::get_offsets", "open", fn);
            return -1;
        }
        FpKeeper keeper(&fp);

        char blk1[M_o_b1size];
        if (fread(blk1, M_o_b1size, 1, fp) != 1) {
            LOGSYSERR("MboxCache::get_offsets", "read blk1", "");
            return -1;
        }
        ConfSimple cf(string(blk1, M_o_b1size));
        string fudi;
        if (!cf.get("udi", fudi) || fudi.compare(udi)) {
            LOGINFO("MboxCache::get_offset:badudi fn " << fn << " udi ["
                    << udi << "], fudi [" << fudi << "]\n");
            return -1;
        }
        LOGDEB1("MboxCache::get_offsets: reading offsets file at offs "
                << cacheoffset(msgnum) << "\n");
        if (fseeko(fp, cacheoffset(msgnum), SEEK_SET) != 0) {
            LOGSYSERR("MboxCache::get_offsets", "seek",
                      lltodecstr(cacheoffset(msgnum)));
            return -1;
        }
        int64_t offset = -1;
        size_t ret;
        if ((ret = fread(&offset, sizeof(int64_t), 1, fp)) != 1) {
            LOGSYSERR("MboxCache::get_offsets", "read", "");
            return -1;
        }
        LOGDEB0("MboxCache::get_offsets: ret " << offset << "\n");
        return offset;
    }

    // Save array of offsets for a given file, designated by Udi
    void put_offsets(RclConfig *config, const string& udi, int64_t fsize,
                     vector<int64_t>& offs) {
        LOGDEB0("MboxCache::put_offsets: " << offs.size() << " offsets\n");
        if (!ok(config) || !maybemakedir())
            return;
        if (fsize < m_minfsize) {
            LOGDEB0("MboxCache::put_offsets: fsize " << fsize << " < minsize " <<
                    m_minfsize << endl);
            return;
        }
        std::unique_lock<std::mutex> locker(o_mcache_mutex);
        string fn = makefilename(udi);
        FILE *fp;
        if ((fp = fopen(fn.c_str(), "wb")) == 0) {
            LOGSYSERR("MboxCache::put_offsets", "fopen", fn);
            return;
        }
        FpKeeper keeper(&fp);
        string blk1("udi=");
        blk1.append(udi);
        blk1.append(cstr_newline);
        blk1.resize(M_o_b1size, 0);
        if (fwrite(blk1.c_str(), M_o_b1size, 1, fp) != 1) {
            LOGSYSERR("MboxCache::put_offsets", "fwrite blk1", "");
            return;
        }

        for (const auto& off : offs) {
            LOGDEB1("MboxCache::put_offsets: writing value " << off <<
                    " at offset " << ftello(fp) << endl);
            if (fwrite((char*)&off, sizeof(int64_t), 1, fp) != 1) {
                LOGSYSERR("MboxCache::put_offsets", "fwrite", "");
                return;
            }
        }
    }

    // Check state, possibly initialize
    bool ok(RclConfig *config) {
        std::unique_lock<std::mutex> locker(o_mcache_mutex);
        if (m_minfsize == -1)
            return false;
        if (!m_ok) {
            int minmbs = 5;
            config->getConfParam("mboxcacheminmbs", &minmbs);
            if (minmbs < 0) {
                // minmbs set to negative to disable cache
                m_minfsize = -1;
                return false;
            }
            m_minfsize = minmbs * 1000 * 1000;

            m_dir = config->getMboxcacheDir();
            m_ok = true;
        }
        return m_ok;
    }

private:
    bool m_ok{false};
    // Place where we store things
    string m_dir;
    // Don't cache smaller files. If -1, don't do anything.
    int64_t m_minfsize{0};

    // Create the cache directory if it does not exist
    bool maybemakedir() {
        if (!path_makepath(m_dir, 0700)) {
            LOGSYSERR("MboxCache::maybemakedir", "path_makepath", m_dir);
            return false;
        }
        return true;
    }
    // Compute file name from udi
    string makefilename(const string& udi) {
        string digest, xdigest;
        MD5String(udi, digest);
        MD5HexPrint(digest, xdigest);
        return path_cat(m_dir, xdigest);
    }
    // Compute offset in cache file for the mbox offset of msgnum
    // Msgnums are from 1
    int64_t cacheoffset(int msgnum) {
        return M_o_b1size + (msgnum-1) * sizeof(int64_t);
    }
};

static class MboxCache o_mcache;

static const string cstr_keyquirks("mhmboxquirks");

MimeHandlerMbox::~MimeHandlerMbox()
{
    clear();
}

void MimeHandlerMbox::clear_impl()
{
    m_fn.erase();
    m_ipath.erase();
    if (m_vfp) {
        fclose((FILE *)m_vfp);
        m_vfp = 0;
    }
    m_msgnum = m_lineno = m_fsize = 0;
    m_offsets.clear();
    m_quirks = 0;
}

bool MimeHandlerMbox::set_document_file_impl(const string& mt, const string &fn)
{
    LOGDEB("MimeHandlerMbox::set_document_file(" << fn << ")\n");
    clear_impl();
    m_fn = fn;
    if (m_vfp) {
        fclose((FILE *)m_vfp);
        m_vfp = 0;
    }

    m_vfp = fopen(fn.c_str(), "rb");
    if (m_vfp == nullptr) {
        LOGSYSERR("MimeHandlerMail::set_document_file", "fopen rb", fn);
        return false;
    }
#if defined O_NOATIME && O_NOATIME != 0
    if (fcntl(fileno((FILE *)m_vfp), F_SETFL, O_NOATIME) < 0) {
        // perror("fcntl");
    }
#endif

    m_fsize = path_filesize(fn);
    m_havedoc = true;
    
    // Check for location-based quirks:
    string quirks;
    if (m_config && m_config->getConfParam(cstr_keyquirks, quirks)) {
        if (quirks == "tbird") {
            LOGDEB("MimeHandlerMbox: setting quirks TBIRD\n");
            m_quirks |= MBOXQUIRK_TBIRD;
        }
    }

    // And double check for thunderbird 
    string tbirdmsf = fn + ".msf";
    if (!(m_quirks & MBOXQUIRK_TBIRD) && path_exists(tbirdmsf)) {
        LOGDEB("MimeHandlerMbox: detected unconf'd tbird mbox in " << fn <<"\n");
        m_quirks |= MBOXQUIRK_TBIRD;
    }

    return true;
}

#define LL 20000
typedef char line_type[LL+10];
static inline void stripendnl(line_type& line, int& ll)
{
    ll = int(strlen(line));
    while (ll > 0) {
        if (line[ll-1] == '\n' || line[ll-1] == '\r') {
            line[ll-1] = 0;
            ll--;
        } else 
            break;
    }
}

bool MimeHandlerMbox::tryUseCache(int mtarg)
{
    bool cachefound = false;
    
    int64_t off;
    line_type line;
    LOGDEB0("MimeHandlerMbox::next_doc: mtarg " << mtarg << " m_udi[" <<
            m_udi << "]\n");
    if (m_udi.empty()) {
        goto out;
    }
    if ((off = o_mcache.get_offset(m_config, m_udi, mtarg)) < 0) {
        goto out;
    }
    LOGDEB1("MimeHandlerMbox::next_doc: got offset " << off <<
            " from cache\n");
    if (fseeko((FILE*)m_vfp, off, SEEK_SET) < 0) {
        goto out;
    }
    LOGDEB1("MimeHandlerMbox::next_doc: fseeko ok\n");
    if (!fgets(line, LL, (FILE*)m_vfp)) {
        goto out;
    }
    LOGDEB1("MimeHandlerMbox::next_doc: fgets  ok. line:[" << line << "]\n");

    if ((fromregex(line) ||
         ((m_quirks & MBOXQUIRK_TBIRD) && minifromregex(line)))  ) {
        LOGDEB0("MimeHandlerMbox: Cache: From_ Ok\n");
        fseeko((FILE*)m_vfp, off, SEEK_SET);
        m_msgnum = mtarg -1;
        cachefound = true;
    } else {
        LOGDEB0("MimeHandlerMbox: cache: regex failed for [" << line << "]\n");
    }

out:
    if (!cachefound) {
        // No cached result: scan.
        fseek((FILE*)m_vfp, 0, SEEK_SET);
        m_msgnum = 0;
    }
    return cachefound;
}

bool MimeHandlerMbox::next_document()
{
    if (nullptr == m_vfp) {
        LOGERR("MimeHandlerMbox::next_document: not open\n");
        return false;
    }
    if (!m_havedoc) {
        return false;
    }
    
    int mtarg = 0;
    if (!m_ipath.empty()) {
        sscanf(m_ipath.c_str(), "%d", &mtarg);
    } else if (m_forPreview) {
        // Can't preview an mbox. 
        LOGDEB("MimeHandlerMbox::next_document: can't preview folders!\n");
        return false;
    }
    LOGDEB0("MimeHandlerMbox::next_document: fn " << m_fn << ", msgnum " <<
            m_msgnum << " mtarg " << mtarg << " \n");

    if (mtarg == 0)
        mtarg = -1;

    // If we are called to retrieve a specific message, try to use the
    // offsets cache to try and position to the right header.
    bool storeoffsets = true;
    if (mtarg > 0) {
        storeoffsets = !tryUseCache(mtarg);
    }

    int64_t message_end = 0;
    int64_t message_end1 = 0;
    bool iseof = false;
    bool hademptyline = true;
    string& msgtxt = m_metaData[cstr_dj_keycontent];
    msgtxt.erase();
    line_type line;
    for (;;) {
        message_end = ftello((FILE*)m_vfp);
        if (!fgets(line, LL, (FILE*)m_vfp)) {
            LOGDEB2("MimeHandlerMbox:next: eof\n");
            iseof = true;
            m_msgnum++;
            break;
        }
        m_lineno++;
        int ll;
        stripendnl(line, ll);
        LOGDEB2("mhmbox:next: hadempty " << hademptyline << " lineno " <<
                m_lineno << " ll " << ll << " Line: [" << line << "]\n");
        if (hademptyline) {
            if (ll > 0) {
                // Non-empty line with empty line flag set, reset flag
                // and check regex.
                if (!(m_quirks & MBOXQUIRK_TBIRD)) {
                    // Tbird sometimes ommits the empty line, so avoid
                    // resetting state (initially true) and hope for
                    // the best
                    hademptyline = false;
                }
                /* The 'F' compare is redundant but it improves performance
                   A LOT */
                if (line[0] == 'F' && (
                        fromregex(line) || 
                        ((m_quirks & MBOXQUIRK_TBIRD) && minifromregex(line)))
                    ) {
                    LOGDEB0("MimeHandlerMbox: msgnum " << m_msgnum <<
                            ", From_ at line " << m_lineno << " foffset " <<
                            message_end << " line: [" << line << "]\n");
                    
                    if (storeoffsets) {
                        m_offsets.push_back(message_end);
                    }
                    m_msgnum++;
                    if ((mtarg <= 0 && m_msgnum > 1) || 
                        (mtarg > 0 && m_msgnum > mtarg)) {
                        // Got message, go do something with it
                        break;
                    }
                    // From_ lines are not part of messages
                    continue;
                } 
            }
        } else if (ll <= 0) {
            hademptyline = true;
        }

        if (mtarg <= 0 || m_msgnum == mtarg) {
            // Accumulate message lines
            line[ll] = '\n';
            line[ll+1] = 0;
            msgtxt += line;
            if (msgtxt.size() > max_mbox_member_size) {
                LOGERR("mh_mbox: huge message (more than " <<
                       max_mbox_member_size/(1024*1024) << " MB) inside " <<
                       m_fn << ", giving up\n");
                return false;
            }
        }
    }
    LOGDEB2("Message text length " << msgtxt.size() << "\n");
    LOGDEB2("Message text: [" << msgtxt << "]\n");
    char buf[20];
    // m_msgnum was incremented when hitting the next From_ or eof, so the data
    // is for m_msgnum - 1
    sprintf(buf, "%d", m_msgnum - 1); 
    m_metaData[cstr_dj_keyipath] = buf;
    m_metaData[cstr_dj_keymt] = "message/rfc822";
    if (iseof) {
        LOGDEB2("MimeHandlerMbox::next: eof hit\n");
        m_havedoc = false;
        if (!m_udi.empty() && storeoffsets) {
            o_mcache.put_offsets(m_config, m_udi, m_fsize, m_offsets);
        }
    }
    return msgtxt.empty() ? false : true;
}
