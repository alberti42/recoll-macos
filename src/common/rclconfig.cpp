#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.56 2007-12-13 06:58:21 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
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
#ifndef TEST_RCLCONFIG
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <langinfo.h>

#include <set>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef __FreeBSD__
#include <osreldate.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "pathut.h"
#include "rclconfig.h"
#include "conftree.h"
#include "debuglog.h"
#include "smallut.h"
#include "textsplit.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#ifndef MIN
#define MIN(A,B) (((A)<(B)) ? (A) : (B))
#endif
#ifndef MAX
#define MAX(A,B) (((A)>(B)) ? (A) : (B))
#endif

RclConfig::RclConfig(const string *argcnf)
{
    zeroMe();
    // Compute our data dir name, typically /usr/local/share/recoll
    const char *cdatadir = getenv("RECOLL_DATADIR");
    if (cdatadir == 0) {
	// If not in environment, use the compiled-in constant. 
	m_datadir = RECOLL_DATADIR;
    } else {
	m_datadir = cdatadir;
    }

    // We only do the automatic configuration creation thing for the default
    // config dir, not if it was specified through -c or RECOLL_CONFDIR
    bool autoconfdir = false;

    // Command line config name overrides environment
    if (argcnf && !argcnf->empty()) {
	m_confdir = path_absolute(*argcnf);
	if (m_confdir.empty()) {
	    m_reason = 
		string("Cant turn [") + *argcnf + "] into absolute path";
	    return;
	}
    } else {
	const char *cp = getenv("RECOLL_CONFDIR");
	if (cp) {
	    m_confdir = cp;
	} else {
	    autoconfdir = true;
	    m_confdir = path_home();
	    m_confdir += ".recoll/";
	}
    }

    if (!autoconfdir) {
	// We want a recoll.conf file to exist in this case to avoid
	// creating indexes all over the place
	string conffile = path_cat(m_confdir, "recoll.conf");
	if (access(conffile.c_str(), 0) < 0) {
	    m_reason = "Explicitely specified configuration must exist"
		" (won't be automatically created)";
	    return;
	}
    }

    if (access(m_confdir.c_str(), 0) < 0) {
	if (!initUserConfig()) 
	    return;
    }

    m_cdirs.push_back(m_confdir);
    m_cdirs.push_back(path_cat(m_datadir, "examples"));
    string cnferrloc = m_confdir + " or " + path_cat(m_datadir, "examples");

    if (!updateMainConfig())
	return;

    mimemap = new ConfStack<ConfTree>("mimemap", m_cdirs, true);
    if (mimemap == 0 || !mimemap->ok()) {
	m_reason = string("No or bad mimemap file in: ") + cnferrloc;
	return;
    }

    mimeconf = new ConfStack<ConfTree>("mimeconf", m_cdirs, true);
    if (mimeconf == 0 || !mimeconf->ok()) {
	m_reason = string("No/bad mimeconf in: ") + cnferrloc;
	return;
    }
    mimeview = new ConfStack<ConfTree>("mimeview", m_cdirs, true);
    if (mimeconf == 0 || !mimeconf->ok()) {
	m_reason = string("No/bad mimeview in: ") + cnferrloc;
	return;
    }

    m_ok = true;
    setKeyDir("");
    return;
}

bool RclConfig::updateMainConfig()
{
    m_conf = new ConfStack<ConfTree>("recoll.conf", m_cdirs, true);
    if (m_conf == 0 || !m_conf->ok()) {
	string where;
	stringsToString(m_cdirs, where);
	m_reason = string("No/bad main configuration file in: ") + where;
	m_ok = false;
	return false;
    }
    setKeyDir("");
    bool nocjk = false;
    if (getConfParam("nocjk", &nocjk) && nocjk == true) {
	TextSplit::cjkProcessing(false);
    } else {
	int ngramlen;
	if (getConfParam("cjkngramlen", &ngramlen)) {
	    TextSplit::cjkProcessing(true, (unsigned int)ngramlen);
	} else {
	    TextSplit::cjkProcessing(true);
	}
    }
    return true;
}

ConfNull *RclConfig::cloneMainConfig()
{
    ConfNull *conf = new ConfStack<ConfTree>("recoll.conf", m_cdirs, false);
    if (conf == 0 || !conf->ok()) {
	m_reason = string("Can't read config");
	return 0;
    }
    return conf;
}

// Remember what directory we're under (for further conf->get()s), and 
// prefetch a few common values.
void RclConfig::setKeyDir(const string &dir) 
{
    m_keydir = dir;
    if (m_conf == 0)
	return;

    if (!m_conf->get("defaultcharset", defcharset, m_keydir))
	defcharset.erase();

    getConfParam("guesscharset", &guesscharset);

    string rmtstr;
    if (m_conf->get("indexedmimetypes", rmtstr, m_keydir)) {
	stringtolower(rmtstr);
	if (rmtstr != m_rmtstr) {
	    LOGDEB2(("RclConfig::setKeyDir: rmtstr [%s]\n", rmtstr.c_str()));
	    m_rmtstr = rmtstr;
	    list<string> l;
	    // Yea, no good to go string->list->set. Lazy me.
	    stringToStrings(rmtstr, l);
	    for (list<string>::iterator it = l.begin(); it !=l.end(); it++) {
		m_restrictMTypes.insert(*it);
	    }
	}
    }
}

bool RclConfig::getConfParam(const std::string &name, int *ivp)
{
    string value;
    if (!getConfParam(name, value))
	return false;
    errno = 0;
    long lval = strtol(value.c_str(), 0, 0);
    if (lval == 0 && errno)
	return 0;
    if (ivp)
	*ivp = int(lval);
    return true;
}

bool RclConfig::getConfParam(const std::string &name, bool *bvp)
{
    if (!bvp) 
	return false;

    *bvp = false;
    string s;
    if (!getConfParam(name, s))
	return false;
    *bvp = stringToBool(s);
    return true;
}

list<string> RclConfig::getTopdirs()
{
    list<string> tdl;
    // Retrieve the list of directories to be indexed.
    string topdirs;
    if (!getConfParam("topdirs", topdirs)) {
	LOGERR(("RclConfig::getTopdirs: no top directories in config\n"));
	return tdl;
    }
    if (!stringToStrings(topdirs, tdl)) {
	LOGERR(("RclConfig::getTopdirs: parse error for directory list\n"));
	return tdl;
    }
    for (list<string>::iterator it = tdl.begin(); it != tdl.end(); it++) {
	*it = path_tildexpand(*it);
	*it = path_canon(*it);
    }
    return tdl;
}

// Get charset to be used for transcoding to utf-8 if unspecified by doc
// For document contents:
//   If defcharset was set (from the config or a previous call), use it.
//   Else, try to guess it from the locale
//   Use iso8859-1 as ultimate default
//   defcharset is reset on setKeyDir()
// For filenames, same thing except that we do not use the config file value
// (only the locale).
const string& RclConfig::getDefCharset(bool filename) 
{
    static string localecharset; // This supposedly never changes
    if (localecharset.empty()) {
	const char *cp;
	cp = nl_langinfo(CODESET);
	// We don't keep US-ASCII. It's better to use a superset
	// Ie: me have a C locale and some french file names, and I
	// can't imagine a version of iconv that couldn't translate
	// from iso8859?
	// The 646 thing is for solaris. 
	if (cp && *cp && strcmp(cp, "US-ASCII") 
#ifdef sun
	    && strcmp(cp, "646")
#endif
	    ) {
	    localecharset = string(cp);
	} else {
	    // Note: it seems that all versions of iconv will take
	    // iso-8859. Some won't take iso8859
	    localecharset = string("ISO-8859-1");
	}
	LOGDEB1(("RclConfig::getDefCharset: localecharset [%s]\n",
		localecharset.c_str()));
    }

    if (defcharset.empty()) {
	defcharset = localecharset;
    }

    if (filename) {
	return localecharset;
    } else {
	return defcharset;
    }
}

// Get all known document mime values (for indexing). We get them from
// the mimeconf 'index' submap: values not in there (ie from mimemap
// or idfile) can't possibly belong to documents in the database.
std::list<string> RclConfig::getAllMimeTypes()
{
    std::list<string> lst;
    if (mimeconf == 0)
	return lst;
    //    mimeconf->sortwalk(mtypesWalker, &lst);
    lst = mimeconf->getNames("index");
    lst.sort();
    lst.unique();
    return lst;
}

// Things for suffix comparison. We define a string class and string 
// comparison with suffix-only sensitivity
class SfString {
public:
    SfString(const string& s) : m_str(s) {}
    bool operator==(const SfString& s2) {
	string::const_reverse_iterator r1 = m_str.rbegin(), re1 = m_str.rend(),
	    r2 = s2.m_str.rbegin(), re2 = s2.m_str.rend();
	while (r1 != re1 && r2 != re2) {
	    if (*r1 != *r2) {
		return 0;
	    }
	    ++r1; ++r2;
	}
	return 1;
    }
    string m_str;
};

class SuffCmp {
public:
    int operator()(const SfString& s1, const SfString& s2) {
	//cout << "Comparing " << s1.m_str << " and " << s2.m_str << endl;
	string::const_reverse_iterator 
	    r1 = s1.m_str.rbegin(), re1 = s1.m_str.rend(),
	    r2 = s2.m_str.rbegin(), re2 = s2.m_str.rend();
	while (r1 != re1 && r2 != re2) {
	    if (*r1 != *r2) {
		return *r1 < *r2 ? 1 : 0;
	    }
	    ++r1; ++r2;
	}
	return 0;
    }
};
typedef multiset<SfString, SuffCmp> SuffixStore;

#define STOPSUFFIXES ((SuffixStore *)m_stopsuffixes)

bool RclConfig::inStopSuffixes(const string& fni)
{
    if (m_stopsuffixes == 0) {
	// Need to initialize the suffixes
	if ((m_stopsuffixes = new SuffixStore) == 0) {
	    LOGERR(("RclConfig::inStopSuffixes: out of memory\n"));
	    return false;
	}
	string stp;
	list<string> stoplist;
	if (mimemap && mimemap->get("recoll_noindex", stp, m_keydir)) {
	    stringToStrings(stp, stoplist);
	}
	for (list<string>::const_iterator it = stoplist.begin(); 
	     it != stoplist.end(); it++) {
	    string lower(*it);
	    stringtolower(lower);
	    STOPSUFFIXES->insert(SfString(lower));
	    if (m_maxsufflen < lower.length())
		m_maxsufflen = lower.length();
	}
    }

    // Only need a tail as long as the longest suffix.
    int pos = MAX(0, int(fni.length() - m_maxsufflen));
    string fn(fni, pos);

    stringtolower(fn);
    SuffixStore::const_iterator it = STOPSUFFIXES->find(fn);
    if (it != STOPSUFFIXES->end()) {
	LOGDEB2(("RclConfig::inStopSuffixes: Found (%s) [%s]\n", 
		fni.c_str(), (*it).m_str.c_str()));
	return true;
    } else {
	LOGDEB2(("RclConfig::inStopSuffixes: not found [%s]\n", fni.c_str()));
	return false;
    }
}

string RclConfig::getMimeTypeFromSuffix(const string &suff)
{
    string mtype;
    mimemap->get(suff, mtype, m_keydir);
    return mtype;
}

string RclConfig::getSuffixFromMimeType(const string &mt)
{
    string suffix;
    list<string>sfs = mimemap->getNames("");
    string mt1;
    for (list<string>::const_iterator it = sfs.begin(); 
	 it != sfs.end(); it++) {
	if (mimemap->get(*it, mt1, ""))
	    if (!stringicmp(mt, mt1))
		return *it;
    }
    return "";
}

/** Get list of file categories from mimeconf */
bool RclConfig::getMimeCategories(list<string>& cats) 
{
    if (!mimeconf)
	return false;
    cats = mimeconf->getNames("categories");
    return true;
}

bool RclConfig::isMimeCategory(string& cat) 
{
    list<string>cats;
    getMimeCategories(cats);
    for (list<string>::iterator it = cats.begin(); it != cats.end(); it++) {
	if (!stringicmp(*it,cat))
	    return true;
    }
    return false;
}

/** Get list of mime types for category from mimeconf */
bool RclConfig::getMimeCatTypes(const string& cat, list<string>& tps)
{
    tps.clear();
    if (!mimeconf)
	return false;
    string slist;
    if (!mimeconf->get(cat, slist, "categories"))
	return false;

    stringToStrings(slist, tps);
    return true;
}

string RclConfig::getMimeHandlerDef(const std::string &mtype, bool filtertypes)
{
    string hs;
    if (filtertypes && !m_restrictMTypes.empty()) {
	string mt = mtype;
	stringtolower(mt);
	if (m_restrictMTypes.find(mt) == m_restrictMTypes.end())
	    return hs;
    }
    if (!mimeconf->get(mtype, hs, "index")) {
	LOGDEB1(("getMimeHandler: no handler for '%s'\n", mtype.c_str()));
    }
    return hs;
}

bool RclConfig::getFieldPrefix(const string& fld, string &pfx)
{
    if (!mimeconf->get(fld, pfx, "prefixes")) {
      LOGDEB(("getFieldPrefix: no prefix defined for '%s'\n", fld.c_str()));
      return false;
    }
    return true;
}

string RclConfig::getMimeViewerDef(const string &mtype)
{
    string hs;
    mimeview->get(mtype, hs, "view");
    return hs;
}

bool RclConfig::getMimeViewerDefs(vector<pair<string, string> >& defs)
{
    if (mimeview == 0)
	return false;
    list<string>tps = mimeview->getNames("view");
    for (list<string>::const_iterator it = tps.begin(); it != tps.end();it++) {
	defs.push_back(pair<string, string>(*it, getMimeViewerDef(*it)));
    }
    return true;
}

bool RclConfig::setMimeViewerDef(const string& mt, const string& def)
{
    string pconfname = path_cat(m_confdir, "mimeview");
    // Make sure this exists 
    close(open(pconfname.c_str(), O_CREAT|O_WRONLY, 0600));
    ConfTree tree(pconfname.c_str());
    if (!tree.set(mt, def, "view")) {
	m_reason = string("RclConfig::setMimeViewerDef: cant set value in ")
	    + pconfname;
	return false;
    }

    list<string> cdirs;
    cdirs.push_back(m_confdir);
    cdirs.push_back(path_cat(m_datadir, "examples"));

    delete mimeview;
    mimeview = new ConfStack<ConfTree>("mimeview", cdirs, true);
    if (mimeview == 0 || !mimeview->ok()) {
	m_reason = string("No/bad mimeview in: ") + m_confdir;
	return false;
    }
    return true;
}

/**
 * Return icon name and path
 */
string RclConfig::getMimeIconName(const string &mtype, string *path)
{
    string iconname;
    mimeconf->get(mtype, iconname, "icons");
    if (iconname.empty())
	iconname = "document";

    if (path) {
	string iconsdir;

#if defined (__FreeBSD__) && __FreeBSD_version < 500000
	// gcc 2.95 dies if we call getConfParam here ??
	if (m_conf) m_conf->get(string("iconsdir"), iconsdir, m_keydir);
#else
	getConfParam("iconsdir", iconsdir);
#endif
	if (iconsdir.empty()) {
	    iconsdir = path_cat(m_datadir, "images");
	} else {
	    iconsdir = path_tildexpand(iconsdir);
	}
	*path = path_cat(iconsdir, iconname) + ".png";
    }
    return iconname;
}

string RclConfig::getDbDir()
{
    string dbdir;
    if (!getConfParam("dbdir", dbdir)) {
	LOGERR(("RclConfig::getDbDir: no db directory in configuration\n"));
    } else {
	dbdir = path_tildexpand(dbdir);
	// If not an absolute path, compute relative to config dir
	if (dbdir.at(0) != '/') {
	    LOGDEB1(("Dbdir not abs, catting with confdir\n"));
	    dbdir = path_cat(m_confdir, dbdir);
	}
    }
    LOGDEB1(("RclConfig::getDbDir: dbdir: [%s]\n", dbdir.c_str()));
    return path_canon(dbdir);
}

string RclConfig::getStopfile()
{
    return path_cat(getConfDir(), "stoplist.txt");
}

list<string> RclConfig::getSkippedNames()
{
    list<string> skpl;
    string skipped;
    if (getConfParam("skippedNames", skipped)) {
	stringToStrings(skipped, skpl);
    }
    return skpl;
}

list<string> RclConfig::getSkippedPaths()
{
    list<string> skpl;
    string skipped;
    if (getConfParam("skippedPaths", skipped)) {
	stringToStrings(skipped, skpl);
    }
    // Always add the dbdir and confdir to the skipped paths
    skpl.push_back(getDbDir());
    skpl.push_back(getConfDir());
    for (list<string>::iterator it = skpl.begin(); it != skpl.end(); it++) {
	*it = path_tildexpand(*it);
	*it = path_canon(*it);
    }
    skpl.sort();
    skpl.unique();
    return skpl;
}

list<string> RclConfig::getDaemSkippedPaths()
{
    list<string> skpl = getSkippedPaths();

    list<string> dskpl;
    string skipped;
    if (getConfParam("daemSkippedPaths", skipped)) {
	stringToStrings(skipped, dskpl);
    }

    for (list<string>::iterator it = dskpl.begin(); it != dskpl.end(); it++) {
	*it = path_tildexpand(*it);
	*it = path_canon(*it);
    }
    dskpl.sort();

    skpl.merge(dskpl);
    skpl.unique();
    return skpl;
}


// Look up an executable filter.  We look in $RECOLL_FILTERSDIR,
// filtersdir in config file, then let the system use the PATH
string RclConfig::findFilter(const string &icmd)
{
    // If the path is absolute, this is it
    if (icmd[0] == '/')
	return icmd;

    string cmd;
    const char *cp;

    // Filters dir from environment ?
    if ((cp = getenv("RECOLL_FILTERSDIR"))) {
	cmd = path_cat(cp, icmd);
	if (access(cmd.c_str(), X_OK) == 0)
	    return cmd;
    } 
    // Filters dir as configuration parameter?
    if (getConfParam(string("filtersdir"), cmd)) {
	cmd = path_cat(cmd, icmd);
	if (access(cmd.c_str(), X_OK) == 0)
	    return cmd;
    } 

    // Filters dir as datadir subdir. Actually the standard case, but
    // this is normally the same value found in config file (previous step)
    cmd = path_cat(m_datadir, "filters");
    cmd = path_cat(cmd, icmd);
    if (access(cmd.c_str(), X_OK) == 0)
	return cmd;

    // Last resort for historical reasons: check in personal config
    // directory
    cmd = path_cat(getConfDir(), icmd);
    if (access(cmd.c_str(), X_OK) == 0)
	return cmd;

    // Let the shell try to find it...
    return icmd;
}

/** 
 * Return decompression command line for given mime type
 */
bool RclConfig::getUncompressor(const string &mtype, list<string>& cmd)
{
    string hs;

    mimeconf->get(mtype, hs, "");
    if (hs.empty())
	return false;
    list<string> tokens;
    stringToStrings(hs, tokens);
    if (tokens.empty()) {
	LOGERR(("getUncompressor: empty spec for mtype %s\n", mtype.c_str()));
	return false;
    }
    list<string>::iterator it = tokens.begin();
    if (tokens.size() < 2)
	return false;
    if (stringlowercmp("uncompress", *it++)) 
	return false;
    cmd.clear();
    cmd.push_back(findFilter(*it++));
    cmd.insert(cmd.end(), it, tokens.end());
    return true;
}

static const char blurb0[] = 
"# The system-wide configuration files for recoll are located in:\n"
"#   %s\n"
"# The default configuration files are commented, you should take a look\n"
"# at them for an explanation of what can be set (you could also take a look\n"
"# at the manual instead).\n"
"# Values set in this file will override the system-wide values for the file\n"
"# with the same name in the central directory. The syntax for setting\n"
"# values is identical.\n"
    ;

// Create initial user config by creating commented empty files
static const char *configfiles[] = {"recoll.conf", "mimemap", "mimeconf", 
				    "mimeview"};
static int ncffiles = sizeof(configfiles) / sizeof(char *);
bool RclConfig::initUserConfig()
{
    // Explanatory text
    char blurb[sizeof(blurb0)+1025];
    string exdir = path_cat(m_datadir, "examples");
    sprintf(blurb, blurb0, exdir.c_str());

    // Use protective 700 mode to create the top configuration
    // directory: documents can be reconstructed from index data.
    if (access(m_confdir.c_str(), 0) < 0 && 
	mkdir(m_confdir.c_str(), 0700) < 0) {
	m_reason += string("mkdir(") + m_confdir + ") failed: " + 
	    strerror(errno);
	return false;
    }
    for (int i = 0; i < ncffiles; i++) {
	string dst = path_cat(m_confdir, string(configfiles[i])); 
	if (access(dst.c_str(), 0) < 0) {
	    FILE *fp = fopen(dst.c_str(), "w");
	    if (fp) {
		fprintf(fp, "%s\n", blurb);
		fclose(fp);
	    } else {
		m_reason += string("fopen ") + dst + ": " + strerror(errno);
		return false;
	    }
	}
    }
    return true;
}

void RclConfig::freeAll() 
{
    delete m_conf;
    delete mimemap;
    delete mimeconf; 
    delete mimeview; 
    delete STOPSUFFIXES;
    // just in case
    zeroMe();
}

void RclConfig::initFrom(const RclConfig& r)
{
    zeroMe();
    if (!(m_ok = r.m_ok))
	return;
    m_reason = r.m_reason;
    m_confdir = r.m_confdir;
    m_datadir = r.m_datadir;
    m_keydir = r.m_datadir;
    // We should use reference-counted objects instead!
    if (r.m_conf)
	m_conf = new ConfStack<ConfTree>(*(r.m_conf));
    if (r.mimemap)
	mimemap = new ConfStack<ConfTree>(*(r.mimemap));
    if (r.mimeconf)
	mimeconf = new ConfStack<ConfTree>(*(r.mimeconf));
    if (r.mimeview)
	mimeview = new ConfStack<ConfTree>(*(r.mimeview));
    if (r.m_stopsuffixes)
	m_stopsuffixes = new SuffixStore(*((SuffixStore*)r.m_stopsuffixes));
    m_maxsufflen = r.m_maxsufflen;
    defcharset = r.defcharset;
    guesscharset = r.guesscharset;
    m_rmtstr = r.m_rmtstr;
    m_restrictMTypes = r.m_restrictMTypes;
}

#else // -> Test

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>

#include <iostream>
#include <list>
#include <string>

using namespace std;

#include "debuglog.h"
#include "rclinit.h"
#include "rclconfig.h"

static char *thisprog;

static char usage [] =
"  \n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s [-s subkey] [-q param]", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s	  0x2 
#define OPT_q	  0x4 

int main(int argc, char **argv)
{
    string pname, skey;
    
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 's':	op_flags |= OPT_s; if (argc < 2)  Usage();
		skey = *(++argv);
		argc--; 
		goto b1;
	    case 'q':	op_flags |= OPT_q; if (argc < 2)  Usage();
		pname = *(++argv);
		argc--; 
		goto b1;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if (argc != 0)
	Usage();

    string reason;
    RclConfig *config = recollinit(0, 0, reason);
    if (config == 0 || !config->ok()) {
	cerr << "Configuration problem: " << reason << endl;
	exit(1);
    }
    if (op_flags & OPT_s)
	config->setKeyDir(skey);
    if (op_flags & OPT_q) {
	string value;
	if (!config->getConfParam(pname, value)) {
	    fprintf(stderr, "getConfParam failed for [%s]\n", pname.c_str());
	    exit(1);
	}
	printf("[%s] -> [%s]\n", pname.c_str(), value.c_str());
    } else {
	list<string> names = config->getConfNames("");
	names.sort();
	names.unique();
	for (list<string>::iterator it = names.begin(); 
	     it != names.end();it++) {
	    string value;
	    config->getConfParam(*it, value);
	    cout << *it << " -> [" << value << "]" << endl;
	}
    }
    exit(0);
}

#endif // TEST_RCLCONFIG
