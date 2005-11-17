#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.11 2005-11-17 12:47:03 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <unistd.h>
#include <errno.h>

#include <iostream>

#include "rclconfig.h"
#include "pathut.h"
#include "conftree.h"
#include "debuglog.h"

using namespace std;

RclConfig::RclConfig()
    : m_ok(false), conf(0), mimemap(0), mimeconf(0)
{
    static int loginit = 0;
    if (!loginit) {
	DebugLog::setfilename("stderr");
	DebugLog::getdbl()->setloglevel(10);
	loginit = 1;
    }

    const char *cp = getenv("RECOLL_CONFDIR");
    if (cp) {
	confdir = cp;
    } else {
	confdir = path_home();
	confdir += ".recoll/";
    }
    string cfilename = confdir;
    path_cat(cfilename, "recoll.conf");

    // Open readonly here so as not to casually create a config file
    conf = new ConfTree(cfilename.c_str(), true);
    if (conf == 0 || 
	(conf->getStatus() != ConfSimple::STATUS_RO && 
	 conf->getStatus() != ConfSimple::STATUS_RW)) {
	reason = string("No main configuration file: ") + cfilename + 
	    " does not exist or cannot be parsed";
	return;
    }

    string mimemapfile;
    if (!conf->get("mimemapfile", mimemapfile, "")) {
	mimemapfile = "mimemap";
    }
    string mpath  = confdir;
    path_cat(mpath, mimemapfile);
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
    if (!conf->get("mimeconffile", mimeconffile, "")) {
	mimeconffile = "mimeconf";
    }
    mpath = confdir;
    path_cat(mpath, mimeconffile);
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
bool RclConfig::getConfParam(const std::string &name, bool *value)
{
    int ival;
    if (!getConfParam(name, &ival))
	return false;
    if (*value)
	*value = ival ? true : false;
    return true;
}

static ConfSimple::WalkerCode mtypesWalker(void *l, 
					   const char *nm, const char *value)
{
    std::list<string> *lst = (std::list<string> *)l;
    if (nm && nm[0] == '.')
	lst->push_back(value);
    return ConfSimple::WALK_CONTINUE;
}

#include "idfile.h"
std::list<string> RclConfig::getAllMimeTypes()
{
    std::list<string> lst;
    if (mimemap == 0)
	return lst;
    mimemap->sortwalk(mtypesWalker, &lst);
    std::list<string> l1 = idFileAllTypes(); 
    lst.insert(lst.end(), l1.begin(), l1.end());
    lst.sort();
    lst.unique();
    return lst;
}

// Look up an executable filter.
// We look in RECOLL_BINDIR, RECOLL_CONFDIR, then let the system use
// the PATH
string find_filter(RclConfig *conf, const string &icmd)
{
    // If the path is absolute, this is it
    if (icmd[0] == '/')
	return icmd;

    string cmd;
    const char *cp;
    if (cp = getenv("RECOLL_BINDIR")) {
	cmd = cp;
	path_cat(cmd, icmd);
	if (access(cmd.c_str(), X_OK) == 0)
	    return cmd;
    } else {
	cmd = conf->getConfDir();
	path_cat(cmd, icmd);
	if (access(cmd.c_str(), X_OK) == 0)
	    return cmd;
    }
    return icmd;
}

