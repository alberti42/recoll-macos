#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.28 2006-04-20 09:20:09 dockes Exp $ (C) 2004 J.F.Dockes";
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
#include <stdio.h>
#include <errno.h>
#include <langinfo.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef __FreeBSD__
#include <osreldate.h>
#endif

#include <iostream>

#include "pathut.h"
#include "rclconfig.h"
#include "conftree.h"
#include "debuglog.h"
#include "smallut.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

RclConfig::RclConfig()
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

    const char *cp = getenv("RECOLL_CONFDIR");
    if (cp) {
	m_confdir = cp;
    } else {
	m_confdir = path_home();
	m_confdir += ".recoll/";
    }

    if (access(m_confdir.c_str(), 0) < 0) {
	if (!initUserConfig())
	    return;
    }

    list<string>cfns;
    string cpath;

    cpath = path_cat(m_confdir, "recoll.conf");
    cfns.push_back(cpath);
    cpath = path_cat(m_datadir, "examples/recoll.conf");
    cfns.push_back(cpath);
    m_conf = new ConfStack<ConfTree>(cfns, true);
    if (m_conf == 0 || !m_conf->ok()) {
	m_reason = string("No main configuration file: ");
	for (list<string>::const_iterator it = cfns.begin(); it != cfns.end();
	     it++) 
	    m_reason += "[" + *it + "] ";
	m_reason += " do not exist or cannot be parsed";
	return;
    }

    cfns.clear();
    cpath  = path_cat(m_confdir, "mimemap");
    cfns.push_back(cpath);
    cpath = path_cat(m_datadir, "examples/mimemap");
    cfns.push_back(cpath);
    mimemap = new ConfStack<ConfTree>(cfns, true);
    if (mimemap == 0 || !mimemap->ok()) {
	m_reason = string("No mime map configuration file: ");
	for (list<string>::const_iterator it = cfns.begin(); it != cfns.end();
	     it++) 
	    m_reason += "[" + *it + "] ";
	m_reason += " do not exist or cannot be parsed";
	return;
    }

    cfns.clear();
    cpath = path_cat(m_confdir, "mimeconf");
    cfns.push_back(cpath);
    cpath = path_cat(m_datadir, "examples/mimeconf");
    cfns.push_back(cpath);
    mimeconf = new ConfStack<ConfTree>(cfns, true);
    if (mimeconf == 0 || !mimeconf->ok()) {
	m_reason = string("No mime configuration file: ");
	for (list<string>::const_iterator it = cfns.begin(); it != cfns.end();
	     it++) 
	    m_reason += "[" + *it + "] ";
	m_reason += " do not exist or cannot be parsed";
	return;
    }

    setKeyDir("");

    m_ok = true;
    return;
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

// Get all known document mime values. We get them from the mimeconf
// 'index' submap: values not in there (ie from mimemap or idfile) can't
// possibly belong to documents in the database.
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

bool RclConfig::getStopSuffixes(list<string>& sufflist)
{
    if (stopsuffixes == 0 && (stopsuffixes = new list<string>) != 0) {
	string stp;
	if (mimemap && mimemap->get("recoll_noindex", stp, m_keydir)) {
	    stringToStrings(stp, *stopsuffixes);
	}
    }

    if (stopsuffixes) {
	sufflist = *stopsuffixes;
	return true;
    }
    return false;
}

string RclConfig::getMimeTypeFromSuffix(const string &suff)
{
    string mtype;
    mimemap->get(suff, mtype, m_keydir);
    return mtype;
}

string RclConfig::getMimeHandlerDef(const std::string &mtype)
{
    string hs;
    if (!mimeconf->get(mtype, hs, "index")) {
	LOGDEB(("getMimeHandler: no handler for '%s'\n", mtype.c_str()));
    }
    return hs;
}

string RclConfig::getMimeViewerDef(const string &mtype)
{
    string hs;
    mimeconf->get(mtype, hs, "view");
    return hs;
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
    if (stringlowercmp("uncompress", tokens.front())) 
	return false;
    list<string>::iterator it = tokens.begin();
    cmd.assign(++it, tokens.end());
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
static const char *configfiles[] = {"recoll.conf", "mimemap", "mimeconf"};
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
    if (r.stopsuffixes)
	stopsuffixes = new std::list<std::string>(*(r.stopsuffixes));
    defcharset = r.defcharset;
    guesscharset = r.guesscharset;
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

int main(int, const char **)
{
    string reason;
    RclConfig *config = recollinit(0, 0, reason);
    if (config == 0 || !config->ok()) {
	cerr << "Configuration problem: " << reason << endl;
	exit(1);
    }
    list<string> names = config->getConfNames("");
    names.sort();
    names.unique();
    for (list<string>::iterator it = names.begin(); it != names.end();it++) {
	string value;
	config->getConfParam(*it, value);
	cout << *it << " -> [" << value << "]" << endl;
    }
}

#endif // TEST_RCLCONFIG
