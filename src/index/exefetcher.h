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
#ifndef _EXEFETCHER_H_INCLUDED_
#define _EXEFETCHER_H_INCLUDED_

#include "fetcher.h"

class RclConfig;

/** 
 * A fetcher which works by executing external programs, defined in a
 * configuration file.
 * At this point this is only used with the sample python mbox indexer,
 * to show how recoll can work with completely external data extraction code.
 *
 * Configuration: The external indexer sets the 'rclbes' recoll field
 * (backend definition, can be FS or BGL -web- in standard recoll) to
 * a unique value (e.g. MBOX for the python sample). A 'backends' file
 * in the configuration directory then links the 'rclbes' value with
 * commands to execute for fetching the data, which recoll uses at
 * query time for previewing and opening the document.
 */
class EXEDocFetcher : public DocFetcher {
    class Internal;
    EXEDocFetcher(const Internal&);
    virtual ~EXEDocFetcher() {}

    virtual bool fetch(RclConfig* cnf, const Rcl::Doc& idoc, RawDoc& out);
    /** Calls stat to retrieve file signature data */
    virtual bool makesig(RclConfig* cnf, const Rcl::Doc& idoc,std::string& sig);
    friend EXEDocFetcher *exeDocFetcherMake(RclConfig *, const std::string&);
private:
    Internal *m;
};

// Lookup bckid in the config and create an appropriate fetcher.
EXEDocFetcher *exeDocFetcherMake(RclConfig *config, const std::string& bckid);

#endif /* _EXEFETCHER_H_INCLUDED_ */
