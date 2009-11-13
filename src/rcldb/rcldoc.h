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
#ifndef _RCLDOC_H_INCLUDED_
#define _RCLDOC_H_INCLUDED_
/* @(#$Id: rcldoc.h,v 1.10 2008-09-16 08:18:30 dockes Exp $  (C) 2006 J.F.Dockes */

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

    // Transcoded version of the simple file name for SFN-prefixed
    // specific file name indexation
    // Index: set by DbIndexer::processone    
    string utf8fn; 

    // Internal path for multi-doc files. Ascii
    // Set by DbIndexer::processone    
    string ipath;

    // Mime type. Set by FileInterner::internfile
    string mimetype;     

    // File modification time as decimal ascii unix time
    // Set by DbIndexer::processone
    string fmtime;

    // Data reference date (same format). Ie: mail date
    // Possibly set by mimetype-specific handler
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
    
    // File size. Index: Set by caller prior to Db::Add. Query: set by
    // rcldb from index doc data. Historically this always has
    // represented the whole file size (as from stat()), but there
    // would be a need for a 3rd value for multidoc files (file
    // size/doc size/ doc text size)
    string fbytes;       

    // Doc text size. Index: from text.length(). Query: set by rcldb from
    // index doc data.
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

    int pc; // relevancy percentage, used by sortseq, convenience
    unsigned long xdocid; // Opaque: rcldb doc identifier.

    ///////////////////////////////////////////////////////////////////
    void erase() {
	url.erase();
	utf8fn.erase();
	ipath.erase();
	mimetype.erase();
	fmtime.erase();
	dmtime.erase();
	origcharset.erase();
	meta.clear();
	syntabs = false;
	fbytes.erase();
	dbytes.erase();
	sig.erase();

	text.erase();
	pc = 0;
	xdocid = 0;
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
    static const string keyfs;  // file size
    static const string keyds;  // document size
    static const string keysz;  // dbytes if set else fbytes
    static const string keysig; // sig
    static const string keyrr;  // relevancy rating
    static const string keyabs; // abstract
    static const string keyau;  // author
    static const string keytt;  // title
    static const string keykw;  // keywords
    static const string keymd5; // file md5 checksum
    static const string keybcknd; // backend type for data not from the filesys
    // udi back from index. Only set by Rcl::Query::getdoc().
    static const string keyudi;
};


#ifndef NO_NAMESPACES
}
#endif

#endif /* _RCLDOC_H_INCLUDED_ */
