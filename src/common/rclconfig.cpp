#ifndef lint
static char rcsid[] = "@(#$Id: rclconfig.cpp,v 1.2 2004-12-15 15:00:36 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>

#include "rclconfig.h"
#include "pathut.h"
#include "conftree.h"

using namespace std;

RclConfig::RclConfig()
    : m_ok(false), conf(0), mimemap(0), mimeconf(0)
{
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
    mimeconf->list();
    m_ok = true;
    return;
}
