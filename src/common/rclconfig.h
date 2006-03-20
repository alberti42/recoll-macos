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
#ifndef _RCLCONFIG_H_INCLUDED_
#define _RCLCONFIG_H_INCLUDED_
/* @(#$Id: rclconfig.h,v 1.16 2006-03-20 09:51:45 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>

#include "conftree.h"
#include "smallut.h"

class RclConfig {
 public:

    RclConfig();
    bool ok() {return m_ok;}
    const string &getReason() {return m_reason;}
    /** Return the directory where this config is stored */
    string getConfDir() {return m_confdir;}

    /** Set current directory reference, and fetch automatic parameters. */
    void setKeyDir(const string &dir) 
    {
	m_keydir = dir;
	m_conf->get("defaultcharset", defcharset, m_keydir);
	string str;
	m_conf->get("guesscharset", str, m_keydir);
	guesscharset = stringToBool(str);
    }

    /** Get generic configuration parameter according to current keydir */
    bool getConfParam(const string &name, string &value) 
    {
	if (m_conf == 0)
	    return false;
	return m_conf->get(name, value, m_keydir);
    }
    /** Variant with autoconversion to int */
    bool getConfParam(const std::string &name, int *value);
    /** Variant with autoconversion to bool */
    bool getConfParam(const std::string &name, bool *value);
    /** Get default charset for current keydir (was set during setKeydir) */
    const string &getDefCharset();
    /** Get guessCharset for current keydir (was set during setKeydir) */
    bool getGuessCharset() {return guesscharset;}


    /** 
     * Get list of ignored suffixes from mimemap
     *
     * The list is initialized on first call, and not changed for subsequent
     * setKeydirs.
     */
    bool getStopSuffixes(std::list<std::string>& sufflist);

    /** 
     * Check in mimeconf if input mime type is a compressed one, and
     * return command to uncompress if it is.
     *
     * The returned command has substitutable places for input file name 
     * and temp dir name, and will return output name
     */
    bool getUncompressor(const std::string &mtpe, std::list<std::string>& cmd);

    /** Use mimemap to compute mimetype */
    std::string getMimeTypeFromSuffix(const std::string &suffix);

    /** Get input filter from mimeconf for mimetype */
    std::string getMimeHandlerDef(const std::string &mimetype);

    /** Get external viewer exec string from mimeconf for mimetype */
    std::string getMimeViewerDef(const std::string &mimetype);

    /** Get icon name from mimeconf for mimetype */
    string getMimeIconName(const string &mtype, string *path = 0);

    /** Get a list of all indexable mime types defined in mimemap */
    std::list<string> getAllMimeTypes();

    /** Find exec file for external filter. cmd is the command name from the
     * command string returned by getMimeHandlerDef */
    std::string findFilter(const std::string& cmd);

    ~RclConfig() {
	freeAll();
    }

    RclConfig(const RclConfig &r) {
	initFrom(r);
    }
    RclConfig& operator=(const RclConfig &r) {
	if (this != &r) {
	    freeAll();
	    initFrom(r);
	}
	return *this;
    }
	
 private:
    int m_ok;
    string m_reason;    // Explanation for bad state
    string m_confdir; // Directory where the files are stored
    string m_datadir; // Example: /usr/local/share/recoll
    string m_keydir;    // Current directory used for parameter fetches.

    ConfTree *m_conf; // Parsed main configuration
    ConfTree *mimemap;  // These are independant of current keydir. 
    ConfTree *mimeconf; 
    ConfTree *mimemap_local; // 
    std::list<std::string> *stopsuffixes;

    // Parameters auto-fetched on setkeydir
    string defcharset;   // These are stored locally to avoid 
    bool   guesscharset; // They are fetched initially or on setKeydir()

    /** Create initial user configuration */
    bool initUserConfig();

    /** Copy from other */
    void initFrom(const RclConfig& r);
    /** Init pointers to 0 */
    void zeroMe() {
	m_ok = false; 
	m_conf = 0; 
	mimemap = 0; 
	mimeconf = 0; 
	mimemap_local = 0; 
	stopsuffixes = 0;
    }
    /** Free data then zero pointers */
    void freeAll() {
	delete m_conf;
	delete mimemap;
	delete mimeconf; 
	delete mimemap_local;
	delete stopsuffixes;
	// just in case
	zeroMe();
    }
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
