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
/* @(#$Id: rcldoc.h,v 1.6 2008-07-29 06:25:29 dockes Exp $  (C) 2006 J.F.Dockes */

#include <string>
#include <map>

#ifndef NO_NAMESPACES
using std::string;
using std::map;
namespace Rcl {
#endif

/**
 * Dumb holder for document attributes and data
 */
class Doc {
 public:
    // These fields potentially go into the document data record
    // We indicate the routine that sets them up during indexing
    
    // This is just "file://" + binary filename. No transcoding: this
    // is used to access files
    // Index: computed from fn by Db::add caller. Query: from doc data.
    string url;

    // Transcoded version of the simple file name for SFN-prefixed
    // specific file name indexation
    // Indexx: set by DbIndexer::processone    
    string utf8fn; 

    // Internal path for multi-doc files. Ascii
    // Set by DbIndexer::processone    
    string ipath;

    string mimetype;     // Set by FileInterner::internfile
    string fmtime;       // File modification time as decimal ascii unix time
                         // Set by DbIndexer::processone
    string dmtime;       // Data reference date (same format). Ie: mail date
                         // Possibly set by handler
    string origcharset;  // Charset we transcoded from (in case we want back)
                         // Possibly set by handler

    // A map for textual metadata like, author, keywords, abstract, title
    // Entries possibly set by handler. If a field-name to prefix translation 
    // exists, the terms will be indexed with a prefix.
    map<string, string> meta; 

    // Attribute for the "abstract" entry. true if it is just the top
    // of doc, not a native document attribute
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

    // The following fields don't go to the db record
    
    string text; // During indexing only: text returned by input handler will
                 // be split and indexed 

    int pc; // used by sortseq, convenience
    unsigned long xdocid; // Opaque: rcldb doc identifier.

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
};


#ifndef NO_NAMESPACES
}
#endif

#endif /* _RCLDOC_H_INCLUDED_ */
