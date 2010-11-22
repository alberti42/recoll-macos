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
/* @(#$Id: rclconfig.h,v 1.43 2008-11-24 15:47:40 dockes Exp $  (C) 2004 J.F.Dockes */

#include <list>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <map>
#include <set>
#ifndef NO_NAMESPACES
using std::list;
using std::string;
using std::vector;
using std::pair;
using std::set;
using std::map;
using std::set;
#endif

#include "conftree.h"
#include "smallut.h"

class RclConfig;

// A small class used for parameters that need to be computed from the
// config string, and which can change with the keydir. Minimize work
// by using the keydirgen and a saved string to avoid unneeded
// recomputations
class ParamStale {
public:
    RclConfig *parent;
    ConfNull  *conffile;
    string    paramname;
    int       savedkeydirgen;
    string    savedvalue;

    void init(RclConfig *rconf, ConfNull *cnf, const string& nm);
    bool needrecompute();
};

class RclConfig {
 public:

    // Constructor: we normally look for a configuration file, except
    // if this was specified on the command line and passed through
    // argcnf
    RclConfig(const string *argcnf = 0);

    // Main programs must implement this, it avoids having to carry
    // the configuration parameter everywhere. Places where several
    // RclConfig instances might be needed will take care of
    // themselves.
    static RclConfig* getMainConfig();

    // Return a writable clone of the main config. This belongs to the
    // caller (must delete it when done)
    ConfNull *cloneMainConfig();

    /** (re)Read recoll.conf */
    bool updateMainConfig();

    bool ok() {return m_ok;}
    const string &getReason() {return m_reason;}

    /** Return the directory where this configuration is stored */
    string getConfDir() {return m_confdir;}
    /** Get the local value for /usr/local/share/recoll/ */
    const string& getDatadir() {return m_datadir;}

    /** Set current directory reference, and fetch automatic parameters. */
    void setKeyDir(const string &dir);
    string getKeyDir() const {return m_keydir;}

    /** Get generic configuration parameter according to current keydir */
    bool getConfParam(const string &name, string &value) 
    {
	if (m_conf == 0)
	    return false;
	return m_conf->get(name, value, m_keydir);
    }
    /** Variant with autoconversion to int */
    bool getConfParam(const string &name, int *value);
    /** Variant with autoconversion to bool */
    bool getConfParam(const string &name, bool *value);
    /** Variant with conversion to string list/vector
     *  (stringToStrings). Can fail if the string is malformed. */
    bool getConfParam(const string &name, vector<string> *value);
    bool getConfParam(const string &name, list<string> *value);

    /** 
     * Get list of config names under current sk, with possible 
     * wildcard filtering 
     */
    list<string> getConfNames(const char *pattern = 0) {
	return m_conf->getNames(m_keydir, pattern);
    }

    /** Check if name exists anywhere in config */
    bool hasNameAnywhere(const string& nm) 
    {
        return m_conf? m_conf->hasNameAnywhere(nm) : false;
    }


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
    /** Get stoplist file name */
    string getStopfile();

    /** Get list of skipped file names for current keydir */
    list<string>& getSkippedNames();

    /** Get list of skipped paths patterns. Doesn't depend on the keydir */
    list<string> getSkippedPaths();
    /** Get list of skipped paths patterns, daemon version (may add some)
	Doesn't depend on the keydir */
    list<string> getDaemSkippedPaths();

    /** conf: Add local fields to target dic */
    bool addLocalFields(map<string, string> *tgt);

    /** 
     * mimemap: Check if file name should be ignored because of suffix
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

    /** mimemap: compute mimetype */
    string getMimeTypeFromSuffix(const string &suffix);
    /** mimemap: get a list of all indexable mime types defined */
    list<string> getAllMimeTypes();
    /** mimemap: Get appropriate suffix for mime type. This is inefficient */
    string getSuffixFromMimeType(const string &mt);

    /** mimeconf: get input filter for mimetype */
    string getMimeHandlerDef(const string &mimetype, bool filtertypes=false);

    /** For lines like: "name = some value; attr1 = value1; attr2 = val2"
     * Separate the value and store the attributes in a ConfSimple 
     * @param whole the raw value. No way to escape a semi-colon in there.
     */
    bool valueSplitAttributes(const string& whole, string& value, 
                              ConfSimple& attrs);

    /** mimeconf: get icon name for mimetype */
    string getMimeIconName(const string &mtype, string *path = 0);

    /** mimeconf: get list of file categories */
    bool getMimeCategories(list<string>&);
    /** mimeconf: is parameter one of the categories ? */
    bool isMimeCategory(string&);
    /** mimeconf: get list of mime types for category */
    bool getMimeCatTypes(const string& cat, list<string>&);

    /** fields: get field prefix from field name */
    bool getFieldPrefix(const string& fldname, string &pfx);
    /** Get implied meanings for field name (ie: author->[author, from]) */
    bool getFieldSpecialisations(const string& fld, 
				 list<string>& childrens, bool top = true);
    /** Get prefixes for specialisations of field name */
    bool getFieldSpecialisationPrefixes(const string& fld, 
					list<string>& pfxes);
    const set<string>& getStoredFields() {return m_storedFields;}
    /** Get canonic name for possible alias */
    string fieldCanon(const string& fld);
    /** Get xattr name to field names translations */
    const map<string, string>& getXattrToField() {return m_xattrtofld;}
    /** Get value of a parameter inside the "fields" file. Only some filters 
        use this (ie: mh_mail). The information specific to a given filter
        is typically stored in a separate section(ie: [mail]) */
    list<string> getFieldSectNames(const string &sk, const char* = 0);
    bool getFieldConfParam(const string &name, const string &sk, string &value);

    /** mimeview: get/set external viewer exec string(s) for mimetype(s) */
    string getMimeViewerDef(const string &mimetype, const string& apptag);
    bool getMimeViewerDefs(vector<pair<string, string> >&);
    bool setMimeViewerDef(const string& mimetype, const string& cmd);
    /** Check if mime type is designated as needing no uncompress before view
     * (if a file of this type is found compressed). Default is true,
     *  exceptions are found in the nouncompforviewmts mimeview list */
    bool mimeViewerNeedsUncomp(const string &mimetype);

    /** Store/retrieve missing helpers description string */
    string getMissingHelperDesc();
    void storeMissingHelperDesc(const string &s);

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

    friend class ParamStale;

 private:
    int m_ok;
    string m_reason;    // Explanation for bad state
    string m_confdir;   // User directory where the customized files are stored
    string m_datadir;   // Example: /usr/local/share/recoll
    string m_keydir;    // Current directory used for parameter fetches.
    int    m_keydirgen; // To help with knowing when to update computed data.

    list<string> m_cdirs; // directory stack for the confstacks

    ConfStack<ConfTree> *m_conf;   // Parsed configuration files
    ConfStack<ConfTree> *mimemap;  // The files don't change with keydir, 
    ConfStack<ConfSimple> *mimeconf; // but their content may depend on it.
    ConfStack<ConfSimple> *mimeview; // 
    ConfStack<ConfSimple> *m_fields;
    map<string, string>  m_fldtopfx;
    map<string, string>  m_aliastocanon;
    set<string>          m_storedFields;
    map<string, string>  m_xattrtofld;

    void        *m_stopsuffixes;
    unsigned int m_maxsufflen;
    ParamStale   m_stpsuffstate;

    ParamStale   m_skpnstate;
    list<string> m_skpnlist;

    // Parameters auto-fetched on setkeydir
    string defcharset;   // These are stored locally to avoid 
    bool   guesscharset; // They are fetched initially or on setKeydir()
    // Limiting set of mime types to be processed. Normally empty.
    ParamStale    m_rmtstate;
    set<string>   m_restrictMTypes; 

    /** Create initial user configuration */
    bool initUserConfig();
    /** Copy from other */
    void initFrom(const RclConfig& r);
    /** Init pointers to 0 */
    void zeroMe();
    /** Free data then zero pointers */
    void freeAll();
    bool readFieldsConfig(const string& errloc);
};


#endif /* _RCLCONFIG_H_INCLUDED_ */
