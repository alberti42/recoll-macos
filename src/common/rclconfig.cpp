#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.8 2005-10-17 13:36:53 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <unistd.h>

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

    // Maybe we should try to open readonly here as, else, this will 
    // casually create a configuration file
    conf = new ConfTree(cfilename.c_str(), 0);
    if (conf == 0) {
	cerr << "No configuration" << endl;
	return;
    }

    string mimemapfile;
    if (!conf->get("mimemapfile", mimemapfile, "")) {
	mimemapfile = "mimemap";
    }
    string mpath  = confdir;
    path_cat(mpath, mimemapfile);
    mimemap = new ConfTree(mpath.c_str());
    if (mimemap == 0) {
	cerr << "No mime map file" << endl;
	return;
    }
    // mimemap->list();

    string mimeconffile;
    if (!conf->get("mimeconffile", mimeconffile, "")) {
	mimeconffile = "mimeconf";
    }
    mpath = confdir;
    path_cat(mpath, mimeconffile);
    mimeconf = new ConfTree(mpath.c_str());
    if (mimeconf == 0) {
	cerr << "No mime conf file" << endl;
	return;
    }
    //    mimeconf->list();

    setKeyDir(string(""));

    m_ok = true;
    return;
}

static ConfSimple::WalkerCode mtypesWalker(void *l, 
					   const char *nm, const char *value)
{
    std::list<string> *lst = (std::list<string> *)l;
    if (nm && nm[0] == '.')
	lst->push_back(value);
    return ConfSimple::WALK_CONTINUE;
}

std::list<string> RclConfig::getAllMimeTypes()
{
    std::list <string> lst;
    if (mimemap == 0)
	return lst;
    mimemap->sortwalk(mtypesWalker, &lst);
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

