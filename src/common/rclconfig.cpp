#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.19 2006-01-19 17:11:46 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>

#include "pathut.h"
#include "rclconfig.h"
#include "conftree.h"
#include "debuglog.h"
#include "smallut.h"
#include "copyfile.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

static const char *configfiles[] = {"recoll.conf", "mimemap", "mimeconf"};
static int ncffiles = sizeof(configfiles) / sizeof(char *);

static bool createConfig(const string &datadir, string &reason)
{
    // Samples directory
    string exdir = path_cat(datadir, "examples");
    // User's 
    string recolldir = path_tildexpand("~/.recoll");
    if (mkdir(recolldir.c_str(), 0755) < 0) {
	reason += string("mkdir(") + recolldir + ") failed: " + 
	    strerror(errno);
	return false;
    }
    for (int i = 0; i < ncffiles; i++) {
	string src = path_cat((const string&)exdir, string(configfiles[i]));
	string dst = path_cat((const string&)recolldir, string(configfiles[i])); 
	if (!copyfile(src.c_str(), dst.c_str(), reason)) {
	    LOGERR(("Copyfile failed: %s\n", reason.c_str()));
	    return false;
	}
    }
    return true;
}


RclConfig::RclConfig()
    : m_ok(false), m_conf(0), mimemap(0), mimeconf(0), mimemap_local(0),
      stopsuffixes(0)
{
    static int loginit = 0;
    if (!loginit) {
	DebugLog::setfilename("stderr");
	DebugLog::getdbl()->setloglevel(10);
	loginit = 1;
    }

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
    string cfilename = path_cat(m_confdir, "recoll.conf");

    if (access(m_confdir.c_str(), 0) != 0 || 
	access(cfilename.c_str(), 0) != 0) {
	if (!createConfig(m_datadir, reason))
	    return;
    }

    // Open readonly here so as not to casually create a config file
    m_conf = new ConfTree(cfilename.c_str(), true);
    if (m_conf == 0 || 
	(m_conf->getStatus() != ConfSimple::STATUS_RO && 
	 m_conf->getStatus() != ConfSimple::STATUS_RW)) {
	reason = string("No main configuration file: ") + cfilename + 
	    " does not exist or cannot be parsed";
	return;
    }

    string mimemapfile;
    if (!m_conf->get("mimemapfile", mimemapfile, "")) {
	mimemapfile = "mimemap";
    }
    string mpath  = path_cat(m_confdir, mimemapfile);
    mimemap = new ConfTree(mpath.c_str(), true);
    if (mimemap == 0 ||
	(mimemap->getStatus() != ConfSimple::STATUS_RO && 
	 mimemap->getStatus() != ConfSimple::STATUS_RW)) {
	reason = string("No mime map configuration file: ") + mpath + 
	    " does not exist or cannot be parsed";
	return;
    }
    // mimemap->list();

    string mimeconffile;
    if (!m_conf->get("mimeconffile", mimeconffile, "")) {
	mimeconffile = "mimeconf";
    }
    mpath = path_cat(m_confdir, mimeconffile);
    mimeconf = new ConfTree(mpath.c_str(), true);
    if (mimeconf == 0 ||
	(mimeconf->getStatus() != ConfSimple::STATUS_RO && 
	 mimeconf->getStatus() != ConfSimple::STATUS_RW)) {
	reason = string("No mime configuration file: ") + mpath + 
	    " does not exist or cannot be parsed";
	return;
    }
    //    mimeconf->list();

    setKeyDir(string(""));

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
	if (mimemap->get("recoll_noindex", stp, keydir)) {
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
    mimemap->get(suff, mtype, keydir);
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
 * Return icon name
 */
string RclConfig::getMimeIconName(const string &mtype)
{
    string hs;
    mimeconf->get(mtype, hs, "icons");
    return hs;
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
