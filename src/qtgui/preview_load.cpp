/* Copyright (C) 2014 J.F.Dockes
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
#include <string>

#include "debuglog.h"
#include "preview_load.h"
#include "internfile.h"
#include "rcldoc.h"
#include "pathut.h"
#include "cancelcheck.h"
#include "rclconfig.h"

LoadThread::LoadThread(RclConfig *config, const Rcl::Doc& idc,
                       bool pvhtm, QObject *parent)
    : QThread(parent), status(1), m_idoc(idc), m_previewHtml(pvhtm),
      m_config(*config)
{
    // Save log level while we're running in the calling thread.
    m_loglevel = DebugLog::getdbl()->getlevel();
}

void LoadThread::run()
{
    DebugLog::getdbl()->setloglevel(m_loglevel);

    FileInterner interner(m_idoc, &m_config, FileInterner::FIF_forPreview);
    FIMissingStore mst;
    interner.setMissingStore(&mst);

    // Even when previewHtml is set, we don't set the interner's
    // target mtype to html because we do want the html filter to
    // do its work: we won't use the text/plain, but we want the
    // text/html to be converted to utf-8 (for highlight processing)
    try {
        string ipath = m_idoc.ipath;
        FileInterner::Status ret = interner.internfile(fdoc, ipath);
        if (ret == FileInterner::FIDone || ret == FileInterner::FIAgain) {
            // FIAgain is actually not nice here. It means that the record
            // for the *file* of a multidoc was selected. Actually this
            // shouldn't have had a preview link at all, but we don't know
            // how to handle it now. Better to show the first doc than
            // a mysterious error. Happens when the file name matches a
            // a search term.
            status = 0;
            // If we prefer HTML and it is available, replace the
            // text/plain document text
            if (m_previewHtml && !interner.get_html().empty()) {
                fdoc.text = interner.get_html();
                fdoc.mimetype = "text/html";
            }
            tmpimg = interner.get_imgtmp();
        } else {
            fdoc.mimetype = interner.getMimetype();
            mst.getMissingExternal(missing);
            status = -1;
        }
    } catch (CancelExcept) {
        LOGDEB(("LoadThread: cancelled\n"));
        status = -1;
    }
}
