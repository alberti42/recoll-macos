#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.9 2005-11-25 09:13:07 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>

#include "conftree.h"
#include "smallut.h"

class RclConfig {
 public:

    RclConfig();
    ~RclConfig() {delete conf;delete mimemap;delete mimeconf;}

    bool ok() {return m_ok;}
    const string &getReason() {return reason;}
    string getConfDir() {return confdir;}
    //ConfTree *getConfig() {return m_ok ? conf : 0;}

    /// Get generic configuration parameter according to current keydir
    bool getConfParam(const string &name, string &value) 
    {
	if (conf == 0)
	    return false;
	return conf->get(name, value, keydir);
    }

    /* 
     * Variants with autoconversion
     */
    bool getConfParam(const std::string &name, int *value);
    bool getConfParam(const std::string &name, bool *value);

    /// Set current directory reference, and fetch automatic parameters.
    void setKeyDir(const string &dir) 
    {
	keydir = dir;
	conf->get("defaultcharset", defcharset, keydir);
	conf->get("defaultlanguage", deflang, keydir);
	string str;
	conf->get("guesscharset", str, keydir);
	guesscharset = stringToBool(str);
    }

    /** 
     * Check if input mime type is a compressed one, and return command to 
     * uncompress if it is
     * The returned command has substitutable places for input file name 
     * and temp dir name, and will return output name
     */
    bool getUncompressor(const std::string &mtpe, std::list<std::string>& cmd);
    bool getStopSuffixes(std::list<std::string>& sufflist);
    std::string getMimeTypeFromSuffix(const std::string &suffix);
    std::string getMimeHandlerDef(const std::string &mtype);
    /**
     * Return external viewer exec string for given mime type
     */
    std::string getMimeViewerDef(const std::string &mtype);
    /**
     * Return icon name for mime type
     */
    string getMimeIconName(const string &mtype);


    const string &getDefCharset() {return defcharset;}
    const string &getDefLang() {return deflang;}
    bool getGuessCharset() {return guesscharset;}
    std::list<string> getAllMimeTypes();

 private:
    int m_ok;
    string reason;    // Explanation for bad state
    string   confdir; // Directory where the files are stored
    ConfTree *conf;   // Parsed main configuration
    string keydir;    // Current directory used for parameter fetches.
    
    ConfTree *mimemap;  // These are independant of current keydir. 
    ConfTree *mimeconf; 
    ConfTree *mimemap_local; // 
    std::list<std::string> *stopsuffixes;

    // Parameters auto-fetched on setkeydir
    string defcharset;   // These are stored locally to avoid 
    string deflang;      // a config lookup each time.
    bool   guesscharset; // They are fetched initially or on setKeydir()
};

std::string find_filter(RclConfig *conf, const string& cmd);

#endif /* _RCLCONFIG_H_INCLUDED_ */
