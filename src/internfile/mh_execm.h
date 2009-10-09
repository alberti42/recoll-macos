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
#ifndef _MH_EXECM_H_INCLUDED_
#define _MH_EXECM_H_INCLUDED_
/* @(#$Id: mh_exec.h,v 1.8 2008-10-06 06:22:46 dockes Exp $  (C) 2004 J.F.Dockes */

#include "mh_exec.h"
#include "execmd.h"

/** 
 * Turn external document into internal one by executing an external filter.
 *
 * The command to execute, and its parameters, are stored in the "params" 
 * which is built in mimehandler.cpp out of data from the mimeconf file.
 *
 * This version uses persistent filters which can handle multiple requests 
 * without exiting, with a simple question/response protocol.
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
 * Until proven otherwise, this format is both extensible and
 * reasonably easy to parse. While it's more destined for python or
 * perl on the script side, it should even be sort of usable from the shell
 * (ie: use dd to read the counted data). Most alternatives would need data
 * encoding in some cases.
 */
class MimeHandlerExecMultiple : public MimeHandlerExec {
    /////////
    // Things not reset by "clear()", additionally to those in MimeHandlerExec
    ExecCmd  m_cmd;
    /////// End un-cleared stuff.

 public:
    MimeHandlerExecMultiple(const string& mt) 
        : MimeHandlerExec(mt)
    {}
    // No resources to clean up, the ExecCmd destructor does it.
    virtual ~MimeHandlerExecMultiple() {}
    virtual bool next_document();
    virtual void clear() {
	MimeHandlerExec::clear(); 
    }
private:
    bool startCmd();
    bool readDataElement(string& name);
};

#endif /* _MH_EXECM_H_INCLUDED_ */
