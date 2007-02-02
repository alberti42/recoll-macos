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
/* @(#$Id: rclconfig.h,v 1.31 2007-02-02 10:12:58 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>
#include <string>
#include <vector>
#include <utility>
#ifndef NO_NAMESPACES
using std::list;
using std::string;
using std::vector;
using std::pair;
#endif

#include "conftree.h"
#include "smallut.h"

class RclConfig {
 public:

    RclConfig(const string *argcnf = 0);
    bool ok() {return m_ok;}
    const string &getReason() {return m_reason;}
    /** Return the directory where this config is stored */
    string getConfDir() {return m_confdir;}

    /** Set current directory reference, and fetch automatic parameters. */
    void setKeyDir(const string &dir) 
    {
	m_keydir = dir;
	if (!m_conf->get("defaultcharset", defcharset, m_keydir))
	    defcharset.erase();
	string str;
	m_conf->get("guesscharset", str, m_keydir);
	guesscharset = stringToBool(str);
    }
    string getKeyDir() const {return m_keydir;}

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

    /** Get default charset for current keydir (was set during setKeydir) 
     * filenames are handled differently */
    const string &getDefCharset(bool filename = false);
    /** Get guessCharset for current keydir (was set during setKeydir) */
    bool getGuessCharset() {return guesscharset;}

    /** Get list of top directories. This is needed from a number of places
     * and needs some cleaning-up code. An empty list is always an error, no
     * need for other status */
    list<string> getTopdirs();

    /** Get database directory */
    string getDbDir();

    /** Get list of skipped names for current keydir */
    list<string> getSkippedNames();

    /** Get list of skipped paths patterns. Doesn't depend on the keydir */
    list<string> getSkippedPaths();

    /** Get list of skipped paths patterns, daemon version (may add some)
	Doesn't depend on the keydir */
    list<string> getDaemSkippedPaths();

    /** 
     * Check if file name should be ignored because of suffix
     *
     * The list of ignored suffixes is initialized on first call, and
     * not changed for subsequent setKeydirs.
     */
    bool inStopSuffixes(const string& fn);

    /** 
     * Check in mimeconf if input mime type is a compressed one, and
     * return command to uncompress if it is.
     *
     * The returned command has substitutable places for input file name 
     * and temp dir name, and will return output name
     */
    bool getUncompressor(const string &mtpe, list<string>& cmd);

    /** Use mimemap to compute mimetype */
    string getMimeTypeFromSuffix(const string &suffix);

    /** Get appropriate suffix for mime type. This is inefficient */
    string getSuffixFromMimeType(const string &mt);

    /** Get input filter from mimeconf for mimetype */
    string getMimeHandlerDef(const string &mimetype);

    /** Get external viewer exec string from mimeconf for mimetype */
    string getMimeViewerDef(const string &mimetype);
    bool getMimeViewerDefs(vector<pair<string, string> >&);
    bool setMimeViewerDef(const string& mimetype, const string& cmd);

    /** Get icon name from mimeconf for mimetype */
    string getMimeIconName(const string &mtype, string *path = 0);

    /** Get list of file categories from mimeconf */
    bool getMimeCategories(list<string>&);
    /** Get list of mime types for category from mimeconf */
    bool getMimeCatTypes(const string& cat, list<string>&);

    /** Get a list of all indexable mime types defined in mimemap */
    list<string> getAllMimeTypes();

    /** Find exec file for external filter. cmd is the command name from the
     * command string returned by getMimeHandlerDef */
    string findFilter(const string& cmd);

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
    list<string> getConfNames(const string &sk) {
	return m_conf->getNames(sk);
    }

 private:
    int m_ok;
    string m_reason;    // Explanation for bad state
    string m_confdir; // Directory where the files are stored
    string m_datadir; // Example: /usr/local/share/recoll
    string m_keydir;    // Current directory used for parameter fetches.

    ConfStack<ConfTree> *m_conf;   // Parsed configuration files
    ConfStack<ConfTree> *mimemap;  // The files don't change with keydir, 
    ConfStack<ConfTree> *mimeconf; // but their content may depend on it.
    ConfStack<ConfTree> *mimeview; // 

    void        *m_stopsuffixes;
    unsigned int m_maxsufflen;

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
	mimeview = 0; 
	m_stopsuffixes = 0;
	m_maxsufflen = 0;
    }
    /** Free data then zero pointers */
    void freeAll();
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
