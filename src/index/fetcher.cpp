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
#include "autoconfig.h"


#include "log.h"
#include "rclconfig.h"

#include "fetcher.h"
#include "fsfetcher.h"
#include "bglfetcher.h"
#include "exefetcher.h"

DocFetcher *docFetcherMake(RclConfig *config, const Rcl::Doc& idoc)
{
    if (idoc.url.empty()) {
        LOGERR("docFetcherMakeg:: no url in doc!\n" );
        return 0;
    }
    string backend;
    idoc.getmeta(Rcl::Doc::keybcknd, &backend);
    if (backend.empty() || !backend.compare("FS")) {
	return new FSDocFetcher;
#ifndef DISABLE_WEB_INDEXER
    } else if (!backend.compare("BGL")) {
	return new BGLDocFetcher;
#endif
    } else {
        DocFetcher *f = exeDocFetcherMake(config, backend);
        if (!f) {
            LOGERR("DocFetcherFactory: unknown backend ["  << (backend) << "]\n" );
        }
	return f;
    }
}

