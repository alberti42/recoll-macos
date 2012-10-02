/* Copyright (C) 2006 J.F.Dockes
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
#ifndef _RCLDOC_H_INCLUDED_
#define _RCLDOC_H_INCLUDED_

#include <string>
#include <map>

#ifndef NO_NAMESPACES
using std::string;
using std::map;
namespace Rcl {
#endif

/**
 * Dumb holder for document attributes and data. 
 * 
 * This is used both for indexing, where fields are filled-up by the
 * indexer prior to adding to the index, and for querying, where
 * fields are filled from data stored in the index. Not all fields are
 * in use at both index and query times, and not all field data is
 * stored at index time (for example the "text" field is split and
 * indexed, but not stored as such)
 */
class Doc {
 public:
    ////////////////////////////////////////////////////////////
    // The following fields are stored into the document data record (so they
    // can be accessed after a query without fetching the actual document).
    // We indicate the routine that sets them up during indexing
    
    // Binary or url-encoded url. No transcoding: this is used to access files 
    // Index: computed by Db::add caller. 
    // Query: from doc data.
    string url;

    // Internal path for multi-doc files. Ascii
    // Set by FsIndexer::processone    
    string ipath;

    // Mime type. Set by FileInterner::internfile
    string mimetype;     

    // File modification time as decimal ascii unix time
    // Set by FsIndexer::processone
    string fmtime;

    // Data reference date (same format). Ie: mail date
    // Possibly set by mimetype-specific handler
    // Filter::metaData["modificationdate"]
    string dmtime;

    // Charset we transcoded the 'text' field from (in case we want back)
    // Possibly set by handler
    string origcharset;  

    // A map for textual metadata like, author, keywords, abstract,
    // title.  The entries are possibly set by the mimetype-specific
    // handler. If a fieldname-to-prefix translation exists, the
    // terms in the value will be indexed with a prefix.
    // Only some predefined fields are stored in the data record:
    // "title", "keywords", "abstract", "author", but if a field name is
    // in the "stored" configuration list, it will be stored too.
    map<string, string> meta; 

    // Attribute for the "abstract" entry. true if it is just the top
    // of doc, not a native document attribute. Not stored directly, but
    // as an indicative prefix at the beginning of the abstract (ugly hack)
    bool   syntabs;      
    
    // File size. This is the size of the compressed file or of the
    // external containing archive.
    // Index: Set by caller prior to Db::Add. 
    // Query: not set currently (not stored)
    string pcbytes;       

    // Document size, ie, size of the .odt or .xls.
    // Index: Set in internfile from the filter stack
    // Query: set from data record
    string fbytes;

    // Doc text size. 
    // Index: from text.length(). 
    // Query: set by rcldb from index data record
    string dbytes;

    // Doc signature. Used for up to date checks. 
    // Index: set by Db::Add caller. Query: set from doc data.
    // This is opaque to rcldb, and could just as well be ctime, size,
    // ctime+size, md5, whatever.
    string sig;

    /////////////////////////////////////////////////
    // The following fields don't go to the db record, so they can't
    // be retrieved at query time

    // Main document text. This is plaintext utf-8 text to be split
    // and indexed
    string text; 

    /////////////////////////////////////////////////
    // Misc stuff

    int pc; // relevancy percentage, used by sortseq, convenience
    unsigned long xdocid; // Opaque: rcldb doc identifier.

    // Page breaks were stored during indexing.
    bool haspages; 

    ///////////////////////////////////////////////////////////////////

    void erase() {
	url.erase();
	ipath.erase();
	mimetype.erase();
	fmtime.erase();
	dmtime.erase();
	origcharset.erase();
	meta.clear();
	syntabs = false;
	pcbytes.erase();
	fbytes.erase();
	dbytes.erase();
	sig.erase();

	text.erase();
	pc = 0;
	xdocid = 0;
	haspages = false;
    }
    Doc()
     : syntabs(false), pc(0), xdocid(0), haspages(false)
    {
    }
    /** Get value for named field. If value pointer is 0, just test existence */
    bool getmeta(const string& nm, string *value = 0) const
    {
	map<string,string>::const_iterator it = meta.find(nm);
	if (it != meta.end()) {
	    if (value)
		*value = it->second;
	    return true;
	} else {
	    return false;
	}
    }
    bool peekmeta(const string& nm, const string **value = 0) const
    {
	map<string,string>::const_iterator it = meta.find(nm);
	if (it != meta.end()) {
	    if (value)
		*value = &(it->second);
	    return true;
	} else {
	    return false;
	}
    }

    void dump(bool dotext=false) const;

    // The official names for recoll native fields when used in a text
    // context (ie: the python interface duplicates some of the fixed
    // fields in the meta array, these are the names used). Defined in
    // rcldoc.cpp. For fields stored in the meta[] array (ie, title,
    // author), filters _must_ use these values
    static const string keyurl; // url
    static const string keyfn;  // file name
    static const string keyipt; // ipath
    static const string keytp;  // mime type
    static const string keyfmt; // file mtime
    static const string keydmt; // document mtime
    static const string keymt;  // mtime dmtime if set else fmtime
    static const string keyoc;  // original charset
    static const string keypcs;  // document outer container size
    static const string keyfs;  // document size
    static const string keyds;  // document text size
    static const string keysz;  // dbytes if set else fbytes else pcbytes
    static const string keysig; // sig
    static const string keyrr;  // relevancy rating
    static const string keycc;  // Collapse count
    static const string keyabs; // abstract
    static const string keyau;  // author
    static const string keytt;  // title
    static const string keykw;  // keywords
    static const string keymd5; // file md5 checksum
    static const string keybcknd; // backend type for data not from the filesys
    // udi back from index. Only set by Rcl::Query::getdoc().
    static const string keyudi;
    static const string keyapptg; // apptag. Set from localfields (fsindexer)
    static const string keybght;  // beagle hit type ("beagleHitType")
};


#ifndef NO_NAMESPACES
}
#endif

#endif /* _RCLDOC_H_INCLUDED_ */
