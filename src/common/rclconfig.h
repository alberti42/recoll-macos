#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.1 2004-12-14 17:50:28 dockes Exp $  (C) 2004 J.F.Dockes */

#include "conftree.h"

class RclConfig {
    int m_ok;
    string confdir; // Directory where the files are stored
    ConfTree *conf; // Parsed main configuration
    string keydir;  // Current directory used for parameter fetches.
    string defcharset; // These are stored locally to avoid a config lookup
    string deflang;    // each time.
    // Note: this will have to change if/when we support per directory maps
    ConfTree *mimemap;
    ConfTree *mimeconf;
 public:
    RclConfig();
    ~RclConfig() {delete conf;delete mimemap;delete mimeconf;}
    bool ok() {return m_ok;}
    ConfTree *getConfig() {return m_ok ? conf : 0;}
    ConfTree *getMimeMap() {return m_ok ? mimemap : 0;}
    ConfTree *getMimeConf() {return m_ok ? mimeconf : 0;}
    bool getConfParam(const string &name, string &value) 
    {
	if (conf == 0)
	    return false;
	return conf->get(name, value, keydir);
    }
    const string &getDefCharset() {
	return defcharset;
    }
    const string &getDefLang() {
	return deflang;
    }
    void setKeyDir(const string &dir) 
    {
	keydir = dir;
	conf->get("defaultcharset", defcharset, keydir);
	conf->get("defaultlanguage", deflang, keydir);
    }
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
