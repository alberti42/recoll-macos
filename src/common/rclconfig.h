#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.2 2004-12-15 15:00:36 dockes Exp $  (C) 2004 J.F.Dockes */

#include "conftree.h"

class RclConfig {
    int m_ok;
    string confdir; // Directory where the files are stored
    ConfTree *conf; // Parsed main configuration
    string keydir;  // Current directory used for parameter fetches.
    // Note: this will have to change if/when we support per directory maps
    ConfTree *mimemap;
    ConfTree *mimeconf;
 public:
    // Let some parameters be accessed directly
    string defcharset; // These are stored locally to avoid a config lookup
    string deflang;    // each time.
    bool   guesscharset;

    RclConfig();
    ~RclConfig() {delete conf;delete mimemap;delete mimeconf;}
    bool ok() {return m_ok;}
    ConfTree *getConfig() {return m_ok ? conf : 0;}
    ConfTree *getMimeMap() {return m_ok ? mimemap : 0;}
    ConfTree *getMimeConf() {return m_ok ? mimeconf : 0;}
    void setKeyDir(const string &dir) 
    {
	keydir = dir;
	conf->get("defaultcharset", defcharset, keydir);
	conf->get("defaultlanguage", deflang, keydir);
	string str;
	conf->get("guesscharset", deflang, str);
	guesscharset = ConfTree::stringToBool(str);
    }
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
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
