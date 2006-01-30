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
#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.11 2006-01-30 11:15:27 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#include "rclconfig.h"
#include "rcldb.h"


/**
 * Document interner class. 
 */
class MimeHandler {
 public:
    MimeHandler() : m_forPreview(false) {}
    virtual ~MimeHandler() {}

    /// Status from mkDoc method.
    enum Status {MHError, MHDone, MHAgain};
    /**
     * Transform external data into internal utf8 document
     *
     * @param conf the global configuration
     * @param filename File from which the data comes from
     * @param mimetype its mime type (from the mimemap configuration file)
     * @param outdoc   The output document
     * @param ipath the access path for the document inside the file. 
     *              For mono-document file types, this will always be empty. 
     *              It is used, for example for mbox files which may contain
     *              multiple emails. If this is not empty in input, then the
     *              caller is requesting a single document (ie: for display).
     *              If this is empty (during indexation), it will be filled-up
     *              by the function, and all the file's documents will be 
     *              returned by successive calls.
     * @return The return value indicates if there are more documents to be 
     *         fetched from the same file.
     */
    virtual MimeHandler::Status mkDoc(RclConfig * conf, 
				      const std::string &filename, 
				      const std::string &mimetype, 
				      Rcl::Doc& outdoc,
				      string& ipath) = 0;

    virtual void setForPreview(bool onoff) {m_forPreview = onoff;};

 protected:
    bool m_forPreview;
};

/**
 * Return indexing handler object for the given mime type
 * returned pointer should be deleted by caller
 */
extern MimeHandler *getMimeHandler(const std::string &mtyp, RclConfig *cfg);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
