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
static char rcsid[] = "@(#$Id: $ (C) 2009 J.F.Dockes";
#endif

#ifndef TEST_CIRCACHE

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <sstream>
#include <iostream>

#include "circache.h"
#include "conftree.h"
#include "debuglog.h"
#include "smallut.h"

using namespace std;

/*
 * File structure:
 * - Starts with a 1-KB header block, with a param dictionary.
 * - Stored items follow. Each item has a header and 2 segments for
 *   the metadata and the data. 
 *   The segment sizes are stored in the ascii header/marker:
 *     circacheSizes = xxx yyy zzz
 *     xxx bytes of metadata
 *     yyy bytes of data
 *     zzz bytes of padding up to next object (only one entry has non zero)
 *
 * There is a write position, which can be at eof while 
 * the file is growing, or inside the file if we are recycling. This is stored
 * in the header, together with the maximum size
 *
 * If we are recycling, we have to take care to compute the size of the 
 * possible remaining area from the last object invalidated by the write, 
 * pad it with neutral data and store the size in the new header.
 */

// First block size
#define CIRCACHE_FIRSTBLOCK_SIZE 1024

// Entry header.
// The 32 bits size are stored as hex integers so the maximum size is
// 13 + 3x8 + 6 = 43
#define CIRCACHE_HEADER_SIZE 50
const char *headerformat = "circacheSizes = %x %x %x";
class EntryHeaderData {
public:
    EntryHeaderData() : dicsize(0), datasize(0), padsize(0) {}
    unsigned int dicsize;
    unsigned int datasize;
    unsigned int padsize;
};

// A callback class for the header-hopping function.
class CCScanHook {
public:
    virtual ~CCScanHook() {}
    enum status {Stop, Continue, Error, Eof};
    virtual status takeone(off_t offs, const string& udi, unsigned int dicsize, 
                           unsigned int datasize, unsigned int padsize) = 0;
};

class CirCacheInternal {
public:
    int m_fd;
    ////// These are cache persistent state and written to the first block:
    // Maximum file size, after which we begin reusing old space
    off_t m_maxsize; 
    // Offset of the oldest header. 
    off_t m_oheadoffs;
    // Offset of last write (newest header)
    off_t m_nheadoffs;
    // Pad size for newest entry. 
    int   m_npadsize;
    ///////////////////// End header entries

    // A place to hold data when reading
    char *m_buffer;
    size_t m_bufsiz;
    // Error messages
    ostringstream m_reason;

    // State for rewind/next/getcurrent operation
    off_t  m_itoffs;
    EntryHeaderData m_ithd;

    CirCacheInternal()
        : m_fd(-1), m_maxsize(-1), m_oheadoffs(-1), 
          m_nheadoffs(0), m_npadsize(0), m_buffer(0), m_bufsiz(0)
    {}

    ~CirCacheInternal()
    {
        if (m_fd >= 0)
            close(m_fd);
        if (m_buffer)
            free(m_buffer);
    }

    char *buf(size_t sz)
    {
        if (m_bufsiz >= sz)
            return m_buffer;
        if ((m_buffer = (char *)realloc(m_buffer, sz))) {
            m_bufsiz = sz;
        } else {
            m_reason << "CirCache:: realloc(" << sz << ") failed";
            m_bufsiz = 0;
        }
        return m_buffer;
    }

    // Name for the cache file
    string datafn(const string& d)
    {
        return  path_cat(d, "circache.crch");
    }

    bool writefirstblock()
    {
        assert(m_fd >= 0);

        ostringstream s;
        s << 
            "maxsize = " << m_maxsize << "\n" <<
            "oheadoffs = " << m_oheadoffs << "\n" <<
            "nheadoffs = " << m_nheadoffs << "\n" <<
            "npadsize = " << m_npadsize << "\n" <<
            "                                                              " <<
            "                                                              " <<
            "\0";

        int sz = int(s.str().size());
        assert(sz < CIRCACHE_FIRSTBLOCK_SIZE);
        lseek(m_fd, 0, 0);
        if (write(m_fd, s.str().c_str(), sz) != sz) {
            m_reason << "writefirstblock: write() failed: errno " << errno;
            return false;
        }
        return true;
    }

    bool readfirstblock()
    {
        assert(m_fd >= 0);

        char *bf = buf(CIRCACHE_FIRSTBLOCK_SIZE);
        if (!bf)
            return false;
        lseek(m_fd, 0, 0);
        if (read(m_fd, bf, CIRCACHE_FIRSTBLOCK_SIZE) != 
            CIRCACHE_FIRSTBLOCK_SIZE) {
            m_reason << "readfirstblock: read() failed: errno " << errno;
            return false;
        }
        string s(bf, CIRCACHE_FIRSTBLOCK_SIZE);
        ConfSimple conf(s, 1);
        string value;
        if (!conf.get("maxsize", value, "")) {
            m_reason << "readfirstblock: conf get maxsize failed";
            return false;
        }
        m_maxsize = atol(value.c_str());
        if (!conf.get("oheadoffs", value, "")) {
            m_reason << "readfirstblock: conf get oheadoffs failed";
            return false;
        }
        m_oheadoffs = atol(value.c_str());
        if (!conf.get("nheadoffs", value, "")) {
            m_reason << "readfirstblock: conf get nheadoffs failed";
            return false;
        }
        m_nheadoffs = atol(value.c_str());
        if (!conf.get("npadsize", value, "")) {
            m_reason << "readfirstblock: conf get npadsize failed";
            return false;
        }
        m_npadsize = atol(value.c_str());
        return true;
    }            

    bool writeentryheader(off_t offset, const EntryHeaderData& d)
    {
        char *bf = buf(CIRCACHE_HEADER_SIZE);
        if (bf == 0)
            return false;
        memset(bf, 0, CIRCACHE_HEADER_SIZE);
        sprintf(bf, headerformat, d.dicsize, d.datasize, d.padsize);
        if (lseek(m_fd, offset, 0) != offset) {
            m_reason << "CirCache::weh: lseek(" << offset << 
                ") failed: errno " << errno;
            return false;
        }
        if (write(m_fd, bf, CIRCACHE_HEADER_SIZE) !=  CIRCACHE_HEADER_SIZE) {
            m_reason << "CirCache::weh: write failed. errno " << errno;
            return false;
        }
        return true;
    }

    CCScanHook::status readentryheader(off_t offset, EntryHeaderData& d)
    {
        assert(m_fd >= 0);

        if (lseek(m_fd, offset, 0) != offset) {
            m_reason << "readentryheader: lseek(" << offset << 
                ") failed: errno " << errno;
            return CCScanHook::Error;
        }
        char *bf = buf(CIRCACHE_HEADER_SIZE);
        if (bf == 0) {
            return CCScanHook::Error;
        }
        int ret = read(m_fd, bf, CIRCACHE_HEADER_SIZE);
        if (ret == 0) {
            // Eof
            m_reason << " Eof ";
            return CCScanHook::Eof;
        }
        if (ret != CIRCACHE_HEADER_SIZE) {
            m_reason << " readheader: read failed errno " << errno;
            return CCScanHook::Error;
        }
        if (sscanf(bf, headerformat, &d.dicsize, &d.datasize, 
                   &d.padsize) != 3) {
            m_reason << " readentryheader: bad header at " << 
                offset << " [" << bf << "]";
            return CCScanHook::Error;
        }
        return CCScanHook::Continue;
    }

    CCScanHook::status scan(off_t startoffset, CCScanHook *user, 
                            bool fold = false)
    {
        assert(m_fd >= 0);

        off_t so0 = startoffset;
        bool already_folded = false;
        
        while (true) {
            if (already_folded && startoffset == so0)
                return CCScanHook::Eof;

            EntryHeaderData d;
            CCScanHook::status st;
            switch ((st = readentryheader(startoffset, d))) {
            case CCScanHook::Continue: break;
            case CCScanHook::Eof:
                if (fold && !already_folded) {
                    already_folded = true;
                    startoffset = CIRCACHE_FIRSTBLOCK_SIZE;
                    continue;
                }
                /* FALLTHROUGH */
            default:
                return st;
            }

            char *bf;
            if ((bf = buf(d.dicsize+1)) == 0) {
                return CCScanHook::Error;
            }
            bf[d.dicsize] = 0;
            if (read(m_fd, bf, d.dicsize) != int(d.dicsize)) {
                m_reason << "scan: read failed errno " << errno;
                return CCScanHook::Error;
            }
            string b(bf, d.dicsize);
            ConfSimple conf(b, 1);
            
            string udi;
            if (!conf.get("udi", udi, "")) {
                m_reason << "scan: no udi in dic";
                return CCScanHook::Error;
            }
                
            // Call callback
            CCScanHook::status a = 
                user->takeone(startoffset, udi, d.dicsize, d.datasize, 
                              d.padsize);
            switch (a) {
            case CCScanHook::Continue: 
                break;
            default:
                return a;
            }
            startoffset += CIRCACHE_HEADER_SIZE + d.dicsize + 
                d.datasize + d.padsize;
        }
    }

    bool readDicData(off_t hoffs, unsigned int dicsize, string& dict, 
                     unsigned int datasize, string* data)
    {
        off_t offs = hoffs + CIRCACHE_HEADER_SIZE;
        if (lseek(m_fd, offs, 0) != offs) {
            m_reason << "CirCache::get: lseek(" << offs << ") failed: " << 
                errno;
            return false;
        }
        char *bf = buf(dicsize);
        if (bf == 0)
            return false;
        if (read(m_fd, bf, dicsize) != int(dicsize)) {
            m_reason << "CirCache::get: read() failed: errno " << errno;
            return false;
        }
        dict.assign(bf, dicsize);
        if (data == 0)
            return true;

        bf = buf(datasize);
        if (bf == 0)
            return false;
        if (read(m_fd, bf, datasize) != int(datasize)){
            m_reason << "CirCache::get: read() failed: errno " << errno;
            return false;
        }
        data->assign(bf, datasize);

        return true;
    }

};

CirCache::CirCache(const string& dir)
    : m_dir(dir)
{
    m_d = new CirCacheInternal;
    LOGDEB(("CirCache: [%s]\n", m_dir.c_str()));
}

CirCache::~CirCache()
{
    delete m_d;
    m_d = 0;
}

string CirCache::getReason()
{
    return m_d ? m_d->m_reason.str() : "Not initialized";
}

bool CirCache::create(off_t m_maxsize, bool onlyifnotexists)
{
    LOGDEB(("CirCache::create: [%s]\n", m_dir.c_str()));
    assert(m_d != 0);
    struct stat st;
    if (stat(m_dir.c_str(), &st) < 0) {
        if (mkdir(m_dir.c_str(), 0777) < 0) {
            m_d->m_reason << "CirCache::create: mkdir(" << m_dir << 
                ") failed" << " errno " << errno;
            return false;
        }
    } else {
        if (onlyifnotexists)
            return open(CC_OPWRITE);
    }

    if ((m_d->m_fd = ::open(m_d->datafn(m_dir).c_str(), 
                          O_CREAT | O_RDWR | O_TRUNC, 
                          0666)) < 0) {
        m_d->m_reason << "CirCache::create: open/creat(" << 
            m_d->datafn(m_dir) << ") failed " << "errno " << errno;
            return false;
    }

    m_d->m_maxsize = m_maxsize;
    m_d->m_oheadoffs = CIRCACHE_FIRSTBLOCK_SIZE;

    char buf[CIRCACHE_FIRSTBLOCK_SIZE];
    memset(buf, 0, CIRCACHE_FIRSTBLOCK_SIZE);
    if (::write(m_d->m_fd, buf, CIRCACHE_FIRSTBLOCK_SIZE) != 
        CIRCACHE_FIRSTBLOCK_SIZE) {
        m_d->m_reason << "CirCache::create: write header failed, errno " 
                      << errno;
        return false;
    }
    return m_d->writefirstblock();
}

bool CirCache::open(OpMode mode)
{
    assert(m_d != 0);
    if (m_d->m_fd >= 0)
        ::close(m_d->m_fd);

    if ((m_d->m_fd = ::open(m_d->datafn(m_dir).c_str(), 
                          mode == CC_OPREAD ? O_RDONLY : O_RDWR)) < 0) {
        m_d->m_reason << "CirCache::open: open(" << m_d->datafn(m_dir) << 
            ") failed " << "errno " << errno;
        return false;
    }
    return m_d->readfirstblock();
}

class CCScanHookDump : public  CCScanHook {
public:
    virtual status takeone(off_t offs, const string& udi, unsigned int dicsize, 
                           unsigned int datasize, unsigned int padsize) 
    {
        cout << "Scan: offs " << offs << " dicsize " << dicsize 
             << " datasize " << datasize << " padsize " << padsize << 
            " udi [" << udi << "]" << endl;
        return Continue;
    }
};

bool CirCache::dump()
{
    CCScanHookDump dumper;
    off_t start = m_d->m_nheadoffs > CIRCACHE_FIRSTBLOCK_SIZE ? 
        m_d->m_nheadoffs : CIRCACHE_FIRSTBLOCK_SIZE;
    switch (m_d->scan(start, &dumper, true)) {
    case CCScanHook::Stop: 
        cout << "Scan returns Stop??" << endl;
        return false;
    case CCScanHook::Continue: 
        cout << "Scan returns Continue ?? " << CCScanHook::Continue << " " <<
            getReason() << endl;
        return false;
    case CCScanHook::Error: 
        cout << "Scan returns Error: " << getReason() << endl;
        return false;
    case CCScanHook::Eof:
        cout << "Scan returns Eof" << endl;
        return true;
    default:
        cout << "Scan returns Unknown ??" << endl;
        return false;
    }
}

class CCScanHookGetter : public  CCScanHook {
public:
    string  m_udi;
    int     m_targinstance;
    int     m_instance;
    off_t   m_offs;
    EntryHeaderData m_hd;

    CCScanHookGetter(const string &udi, int ti)
        : m_udi(udi), m_targinstance(ti), m_instance(0), m_offs(0){}

    virtual status takeone(off_t offs, const string& udi, unsigned int dicsize, 
                           unsigned int datasize, unsigned int padsize) 
    {
        cerr << "offs " << offs << " udi [" << udi << "] dicsize " << dicsize 
             << " datasize " << datasize << " padsize " << padsize << endl;
        if (!m_udi.compare(udi)) {
            m_instance++;
            m_offs = offs;
            m_hd.dicsize = dicsize;
            m_hd.datasize = datasize;
            m_hd.padsize = padsize;
            if (m_instance == m_targinstance)
                return Stop;
        }
        return Continue;
    }
};

// instance == -1 means get latest. Otherwise specify from 1+
bool CirCache::get(const string& udi, string& dict, string& data, int instance)
{
    assert(m_d != 0);
    if (m_d->m_fd < 0) {
        m_d->m_reason << "CirCache::get: not open";
        return false;
    }

    LOGDEB(("CirCache::get: udi [%s], instance\n", udi.c_str(), instance));

    CCScanHookGetter getter(udi, instance);
    off_t start = m_d->m_nheadoffs > CIRCACHE_FIRSTBLOCK_SIZE ? 
        m_d->m_nheadoffs : CIRCACHE_FIRSTBLOCK_SIZE;

    CCScanHook::status ret = m_d->scan(start, &getter, true);
    if (ret == CCScanHook::Eof) {
        if (getter.m_instance == 0)
            return false;
    } else if (ret != CCScanHook::Stop) {
        return false;
    }
    return m_d->readDicData(getter.m_offs, getter.m_hd.dicsize, dict,
                            getter.m_hd.datasize, &data);
}


class CCScanHookSpacer : public  CCScanHook {
public:
    unsigned int sizewanted;
    unsigned int sizeseen;
    
    CCScanHookSpacer(int sz)
        : sizewanted(sz), sizeseen(0) {assert(sz > 0);}

    virtual status takeone(off_t offs, const string& udi, unsigned int dicsize, 
                           unsigned int datasize, unsigned int padsize) 
    {
        LOGDEB(("ScanSpacer: offs %u dicsz %u datasz %u padsz %u udi[%s]\n",
                (unsigned int)offs, dicsize, datasize, padsize, udi.c_str()));
        sizeseen += CIRCACHE_HEADER_SIZE + dicsize + datasize + padsize;
        if (sizeseen >= sizewanted)
            return Stop;
        return Continue;
    }
};

bool CirCache::put(const string& udi, const string& idic, const string& data)
{
    assert(m_d != 0);
    if (m_d->m_fd < 0) {
        m_d->m_reason << "CirCache::put: not open";
        return false;
    }
    string dic = idic;

    // If udi is not already in the metadata, need to add it
    string u;
    ConfSimple conf(dic, 0);
    if (!conf.get("udi", u, "")) {
        ostringstream s;
        conf.set("udi", udi, "");
        conf.write(s);
        dic = s.str();
    }

    struct stat st;
    if (fstat(m_d->m_fd, &st) < 0) {
        m_d->m_reason << "CirCache::put: fstat failed. errno " << errno;
        return false;
    }

    // Characteristics for the new entry
    int nsize = CIRCACHE_HEADER_SIZE + dic.size() + data.size();
    int nwriteoffs = 0;
    int npadsize = 0;
    bool extending = false;

    LOGDEB2(("CirCache::put: nsize %d oheadoffs %d\n", 
             nsize, m_d->m_oheadoffs));

    if (st.st_size < m_d->m_maxsize) {
        // If we are still growing the file, things are simple
        nwriteoffs = lseek(m_d->m_fd, 0, SEEK_END);
        npadsize = 0;
        extending = true;
    } else {
        // We'll write at the oldest header, minus the possible
        // padsize for the previous (latest) one.
        int recovpadsize = m_d->m_oheadoffs == CIRCACHE_FIRSTBLOCK_SIZE ?
            0 : m_d->m_npadsize;
        if (recovpadsize == 0) {
            // No padsize to recover
            nwriteoffs = m_d->m_oheadoffs;
        } else {
            // Need to read the latest entry's header, to rewrite it with a 
            // zero pad size
            EntryHeaderData pd;
            if (m_d->readentryheader(m_d->m_nheadoffs, pd) != 
                CCScanHook::Continue) {
                return false;
            }
            assert(int(pd.padsize) == m_d->m_npadsize);
            LOGDEB2(("CirCache::put: recovering previous padsize %d\n",
                     pd.padsize));
            pd.padsize = 0;
            if (!m_d->writeentryheader(m_d->m_nheadoffs, pd)) {
                return false;
            }
            nwriteoffs = m_d->m_oheadoffs - recovpadsize;
            // If we fail between here and the end, the file is hosed.
        }

        if (nsize <= recovpadsize) {
            // If the new entry fits entirely in the pad area from the
            // latest one, no need to recycle the oldest entries.
            LOGDEB2(("CirCache::put: new fits in old padsize %d\n,"
                     recovpadsize));
            npadsize = recovpadsize - nsize;
        } else {
            // Scan the file until we have enough space for the new entry,
            // and determine the pad size up to the 1st preserved entry
            int scansize = nsize - recovpadsize;
            LOGDEB2(("CirCache::put: scanning for size %d from offs %u\n",
                    scansize, (unsigned int)m_d->m_oheadoffs));
            CCScanHookSpacer spacer(scansize);
            switch (m_d->scan(m_d->m_oheadoffs, &spacer)) {
            case CCScanHook::Stop: 
                LOGDEB2(("CirCache::put: Scan ok, sizeseen %d\n", 
                         spacer.sizeseen));
                npadsize = spacer.sizeseen - scansize;
                break;
            case CCScanHook::Eof:
                // npadsize is 0
                extending = true;
                break;
            case CCScanHook::Continue: 
            case CCScanHook::Error: 
                return false;
            }
        }
    }
    
    LOGDEB2(("CirCache::put: writing %d at %d padsize %d\n", 
             nsize, nwriteoffs, npadsize));
    if (lseek(m_d->m_fd, nwriteoffs, 0) != nwriteoffs) {
        m_d->m_reason << "CirCache::put: lseek failed: " << errno;
        return false;
    }
    char *bf = m_d->buf(CIRCACHE_HEADER_SIZE);
    if (bf == 0) 
        return false;
    memset(bf, 0, CIRCACHE_HEADER_SIZE);
    sprintf(bf, headerformat, dic.size(), data.size(), npadsize);
    struct iovec vecs[3];
    vecs[0].iov_base = bf;
    vecs[0].iov_len = CIRCACHE_HEADER_SIZE;
    vecs[1].iov_base = (void *)dic.c_str();
    vecs[1].iov_len = dic.size();
    vecs[2].iov_base = (void *)data.c_str();
    vecs[2].iov_len = data.size();
    if (writev(m_d->m_fd, vecs, 3) !=  nsize) {
        m_d->m_reason << "put: write failed. errno " << errno;
        if (extending)
            ftruncate(m_d->m_fd, m_d->m_oheadoffs);
        return false;
    }

    // Update first block information
    m_d->m_nheadoffs = nwriteoffs;
    m_d->m_npadsize  = npadsize;
    m_d->m_oheadoffs = nwriteoffs + nsize + npadsize;
    if (nwriteoffs + nsize >= m_d->m_maxsize) {
        // If we are at the biggest allowed size or we are currently
        // growing a young file, the oldest header is at BOT.
        m_d->m_oheadoffs = CIRCACHE_FIRSTBLOCK_SIZE;
    }
    return m_d->writefirstblock();
    return true;
}

bool CirCache::rewind(bool& eof)
{
    assert(m_d != 0);
    eof = false;
    m_d->m_itoffs = m_d->m_oheadoffs;
    CCScanHook::status st = m_d->readentryheader(m_d->m_itoffs, m_d->m_ithd);
    switch(st) {
    case CCScanHook::Eof:
        eof = true;
        return false;
    case CCScanHook::Continue:
        return true;
    default:
        return false;
    }
}

bool CirCache::next(bool& eof)
{
    assert(m_d != 0);

    eof = false;

    m_d->m_itoffs += CIRCACHE_HEADER_SIZE + m_d->m_ithd.dicsize + 
        m_d->m_ithd.datasize + m_d->m_ithd.padsize;
    if (m_d->m_itoffs == m_d->m_oheadoffs) {
        eof = true;
        return false;
    }

    CCScanHook::status st = m_d->readentryheader(m_d->m_itoffs, m_d->m_ithd);
    if (st == CCScanHook::Eof) {
        m_d->m_itoffs = CIRCACHE_FIRSTBLOCK_SIZE;
        if (m_d->m_itoffs == m_d->m_oheadoffs) {
            eof = true;
            return false;
        }
        st = m_d->readentryheader(m_d->m_itoffs, m_d->m_ithd);
    }
    if (st == CCScanHook::Continue)
        return true;
    return false;
}

bool CirCache::getcurrentdict(string& dict)
{
    assert(m_d != 0);
    if (!m_d->readDicData(m_d->m_itoffs, m_d->m_ithd.dicsize, dict, 0, 0))
        return false;
    return true;
}

bool CirCache::getcurrent(string& udi, string& dict, string& data)
{
    assert(m_d != 0);
    if (!m_d->readDicData(m_d->m_itoffs, m_d->m_ithd.dicsize, dict,
                          m_d->m_ithd.datasize, &data))
        return false;

    ConfSimple conf(dict, 1);
    conf.get("udi", udi, "");
    return true;
}

#else // TEST ->

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <iostream>

#include "circache.h"
#include "fileudi.h"
#include "conftree.h"
#include "readfile.h"
#include "debuglog.h"

using namespace std;

static char *thisprog;

static char usage [] =
" -c <dirname> : create\n"
" -p <dirname> <apath> [apath ...] : put files\n"
" -d <dirname> : dump\n"
" -g [-i instance] <dirname> <udi>: get\n"
;
static void
Usage(FILE *fp = stderr)
{
    fprintf(fp, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_c	  0x2 
#define OPT_p     0x8
#define OPT_g     0x10
#define OPT_d     0x20
#define OPT_i     0x40

int main(int argc, char **argv)
{
  int instance = -1;

  thisprog = argv[0];
  argc--; argv++;

  while (argc > 0 && **argv == '-') {
    (*argv)++;
    if (!(**argv))
      /* Cas du "adb - core" */
      Usage();
    while (**argv)
      switch (*(*argv)++) {
      case 'c':	op_flags |= OPT_c; break;
      case 'p':	op_flags |= OPT_p; break;
      case 'g':	op_flags |= OPT_g; break;
      case 'd':	op_flags |= OPT_d; break;
      case 'i':	op_flags |= OPT_i; if (argc < 2)  Usage();
	if ((sscanf(*(++argv), "%d", &instance)) != 1) 
	  Usage(); 
	argc--; 
	goto b1;
      default: Usage();	break;
      }
  b1: argc--; argv++;
  }

  DebugLog::getdbl()->setloglevel(DEBDEB1);
  DebugLog::setfilename("stderr");

  if (argc < 1)
    Usage();
  string dir = *argv++;argc--;

  CirCache cc(dir);

  if (op_flags & OPT_c) {
      if (!cc.create(100*1024)) {
          cerr << "Create failed:" << cc.getReason() << endl;
          exit(1);
      }
  } else if (op_flags & OPT_p) {
      if (argc < 1)
          Usage();
      if (!cc.open(CirCache::CC_OPWRITE)) {
          cerr << "Open failed: " << cc.getReason() << endl;
          exit(1);
      }
      while (argc) {
          string fn = *argv++;argc--;
          char dic[1000];
          sprintf(dic, "#whatever...\nmimetype = text/plain\n");
          string data, reason;
          if (!file_to_string(fn, data, &reason)) {
              cerr << "File_to_string: " << reason << endl;
              exit(1);
          }
          string udi;
          make_udi(fn, "", udi);
   
          if (!cc.put(udi, dic, data)) {
              cerr << "Put failed: " << cc.getReason() << endl;
              exit(1);
          }
      }
      cc.open(CirCache::CC_OPREAD);
  } else if (op_flags & OPT_g) {
      string udi = *argv++;argc--;
      if (!cc.open(CirCache::CC_OPREAD)) {
          cerr << "Open failed: " << cc.getReason() << endl;
          exit(1);
      }
      string dic, data;
      if (!cc.get(udi, dic, data, instance)) {
          cerr << "Get failed: " << cc.getReason() << endl;
          exit(1);
      }
      cout << "Dict: [" << dic << "]" << endl;
  } else if (op_flags & OPT_d) {
      if (!cc.open(CirCache::CC_OPREAD)) {
          cerr << "Open failed: " << cc.getReason() << endl;
          exit(1);
      }
      cc.dump();
  } else
      Usage();

  exit(0);
}

#endif
