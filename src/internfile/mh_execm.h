/* Copyright (C) 2004 J.F.Dockes
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
#ifndef _MH_EXECM_H_INCLUDED_
#define _MH_EXECM_H_INCLUDED_

#include "mh_exec.h"
#include "execmd.h"

/** 
 * Turn external document into internal one by executing an external filter.
 *
 * The command to execute, and its parameters, are stored in the "params" 
 * which is built in mimehandler.cpp out of data from the mimeconf file.
 *
 * This version uses persistent filters which can handle multiple requests 
 * without exiting (both multiple files and multiple documents per file), 
 * with a simple question/response protocol.
 *
 * The data is exchanged in TLV fashion, in a way that should be
 * usable in most script languages. The basic unit has one line with a
 * data type and a count, followed by the data. A 'message' ends with
 * one empty line. A possible exchange:
 * 
 * From recollindex (the message begins before 'Filename'):
 * 
Filename: 24
/my/home/mail/somefolderIpath: 2
22

<Message ends here: because of the empty line after '22'

 * 
 * Example answer:
 * 
Mimetype: 10
text/plainData: 10
0123456789

<Message ends here because of empty line

 *        
 * This format is both extensible and reasonably easy to parse. 
 * While it's more fitted for python or perl on the script side, it
 * should even be sort of usable from the shell (ie: use dd to read
 * the counted data). Most alternatives would need data encoding in
 * some cases.
 *
 * Higher level dialog:
 * The c++ program is the master and sends request messages to the script. The
 * requests have the following fields:
 *  - Filename: the file to process. This can be empty meaning that we 
 *      are requesting the next document in the current file.
 *  - Ipath: this will be present only if we are requesting a specific 
 *      subdocument inside a container file (typically for preview, at query 
 *      time). Absent during indexing (ipaths are generated and sent back from
 *      the script
 *  - Mimetype: this is the mime type for the (possibly container) file. 
 *      Can be useful to filters which handle multiple types, like rclaudio.
 *      
 * The script answers with messages having the following fields:
 *   - Document: translated document data (typically, but not always, html)
 *   - Ipath: ipath for the returned document. Can be used at query time to
 *       extract a specific subdocument for preview. Not present or empty for 
 *       non-container files.
 *   - Mimetype: mime type for the returned data (ie: text/html, text/plain)
 *   - Eofnow: empty field: no document is returned and we're at eof.
 *   - Eofnext: empty field: file ends after the doc returned by this message.
 *   - SubdocError: no subdoc returned by this request, but file goes on.
 *      (the indexer (1.14) treats this as a file-fatal error anyway).
 *   - FileError: error, stop for this file.
 */
class MimeHandlerExecMultiple : public MimeHandlerExec {
    /////////
    // Things not reset by "clear()", additionally to those in MimeHandlerExec
    ExecCmd  m_cmd;
    /////// End un-cleared stuff.

 public:
    MimeHandlerExecMultiple(RclConfig *cnf, const string& mt) 
        : MimeHandlerExec(cnf, mt)
    {}
    // No resources to clean up, the ExecCmd destructor does it.
    virtual ~MimeHandlerExecMultiple() {}
    virtual bool set_document_file(const string &file_path) {
        m_filefirst = true;
        return MimeHandlerExec::set_document_file(file_path);
    }
    virtual bool next_document();

    // skip_to and clear inherited from MimeHandlerExec

private:
    bool startCmd();
    bool readDataElement(string& name, string& data);
    bool m_filefirst;
    int  m_maxmemberkb;
};

#endif /* _MH_EXECM_H_INCLUDED_ */
