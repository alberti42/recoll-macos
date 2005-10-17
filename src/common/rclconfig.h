#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.5 2005-10-17 13:36:53 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>

#include "conftree.h"

class RclConfig {
    int m_ok;
    string   confdir; // Directory where the files are stored
    ConfTree *conf;   // Parsed main configuration
    string keydir;    // Current directory used for parameter fetches.
    
    ConfTree *mimemap;  // These are independant of current keydir. We might
    ConfTree *mimeconf; // want to change it one day.

    // Parameters auto-fetched on setkeydir
    string defcharset;   // These are stored locally to avoid 
    string deflang;      // a config lookup each time.
    bool   guesscharset; // They are fetched initially or on setKeydir()

 public:

    RclConfig();
    ~RclConfig() {delete conf;delete mimemap;delete mimeconf;}

    bool ok() {return m_ok;}

    string getConfDir() {return confdir;}
    ConfTree *getConfig() {return m_ok ? conf : 0;}

    /// Get generic configuration parameter according to current keydir
    bool getConfParam(const string &name, string &value) 
    {
	if (conf == 0)
	    return false;
	return conf->get(name, value, keydir);
    }
    /// Set current directory reference, and fetch automatic parameters.
    void setKeyDir(const string &dir) 
    {
	keydir = dir;
	conf->get("defaultcharset", defcharset, keydir);
	conf->get("defaultlanguage", deflang, keydir);
	string str;
	conf->get("guesscharset", str, keydir);
	guesscharset = ConfTree::stringToBool(str);
    }
    ConfTree *getMimeMap() {return m_ok ? mimemap : 0;}
    ConfTree *getMimeConf() {return m_ok ? mimeconf : 0;}
    const string &getDefCharset() {return defcharset;}
    const string &getDefLang() {return deflang;}
    bool getGuessCharset() {return guesscharset;}
    std::list<string> getAllMimeTypes();
};

std::string find_filter(RclConfig *conf, const string& cmd);

#endif /* _RCLCONFIG_H_INCLUDED_ */
