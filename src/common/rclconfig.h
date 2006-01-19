#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.12 2006-01-19 17:11:46 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>

#include "conftree.h"
#include "smallut.h"

class RclConfig {
 public:

    RclConfig();
    ~RclConfig() {
	delete m_conf;
	delete mimemap;
	delete mimeconf; 
	delete mimemap_local;
	delete stopsuffixes;
    }

    bool ok() {return m_ok;}
    const string &getReason() {return reason;}
    string getConfDir() {return m_confdir;}
    //ConfTree *getConfig() {return m_ok ? conf : 0;}

    /// Get generic configuration parameter according to current keydir
    bool getConfParam(const string &name, string &value) 
    {
	if (m_conf == 0)
	    return false;
	return m_conf->get(name, value, keydir);
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
	m_conf->get("defaultcharset", defcharset, keydir);
	string str;
	m_conf->get("guesscharset", str, keydir);
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
    bool getGuessCharset() {return guesscharset;}
    std::list<string> getAllMimeTypes();

    std::string findFilter(const std::string& cmd);

 private:
    int m_ok;
    string reason;    // Explanation for bad state
    string m_confdir; // Directory where the files are stored
    string m_datadir; // Example: /usr/local/share/recoll
    ConfTree *m_conf; // Parsed main configuration
    string keydir;    // Current directory used for parameter fetches.
    
    ConfTree *mimemap;  // These are independant of current keydir. 
    ConfTree *mimeconf; 
    ConfTree *mimemap_local; // 
    std::list<std::string> *stopsuffixes;

    // Parameters auto-fetched on setkeydir
    string defcharset;   // These are stored locally to avoid 
    bool   guesscharset; // They are fetched initially or on setKeydir()
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
