/* Copyright (C) 2012 J.F.Dockes
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
#ifndef _FETCHER_H_INCLUDED_
#define _FETCHER_H_INCLUDED_

#include <sys/stat.h>
#include <string>

#include "rcldoc.h"

class RclConfig;

/** 
 * Definition for a generic method to retrieve the data
 * for a document designated by its index data (udi/ipath/url).
 * This is used to retrieve the data for previewing. The
 * actual implementation is specific to the kind of backend (file
 * system, beagle cache, others in the future ?), and of course may
 * share code with the indexing-time functions from the specific backend.
 */
class DocFetcher {
public:
    /** A RawDoc is the data for a source document either as a memory
       block, or pointed to by a file name */
    struct RawDoc {
	enum RawDocKind {RDK_FILENAME, RDK_DATA};
	RawDocKind kind;
	std::string data; // Doc data or file name
	struct stat st; // Only used if RDK_FILENAME
    };

    /** 
     * Return the data for the requested document, either as a
     * file-system file or as a memory object (maybe stream too in the
     * future?) 
     * @param cnf the global config
     * @param idoc the data gathered from the index for this doc (udi/ipath)
     * @param out we may return either a file name or the document data. 
     */
    virtual bool fetch(RclConfig* cnf, const Rcl::Doc& idoc, RawDoc& out) = 0;
    
    /** 
     * Return the signature for the requested document. This is used for
     * up-to-date tests performed out of indexing (e.g.: verifying that a 
     * document is not stale before previewing it).
     * @param cnf the global config
     * @param idoc the data gathered from the index for this doc (udi/ipath)
     * @param sig output. 
     */
    virtual bool makesig(RclConfig* cnf, const Rcl::Doc& idoc, string& sig) = 0;
    virtual ~DocFetcher() {}
};

/** Returns an appropriate fetcher object given the backend string identifier */
DocFetcher *docFetcherMake(const Rcl::Doc& idoc);

#endif /* _FETCHER_H_INCLUDED_ */
