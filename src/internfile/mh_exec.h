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
#ifndef _MH_EXEC_H_INCLUDED_
#define _MH_EXEC_H_INCLUDED_

#include <string>
#include <vector>

#include "mimehandler.h"
#include "execmd.h"

class HandlerTimeout {};
    
/** 
 * Turn external document into internal one by executing an external filter.
 *
 * The command to execute, and its parameters, are stored in the "params" 
 * which is built in mimehandler.cpp out of data from the mimeconf file.
 *
 * As any RecollFilter, a MimeHandlerExec object can be reset
 * by calling clear(), and will stay initialised for the same mtype
 * (cmd, params etc.)
 */
class MimeHandlerExec : public RecollFilter {
 public:
    ///////////////////////
    // Members not reset by clear(). params, cfgFilterOutputMtype and 
    // cfgFilterOutputCharset
    // define what I am.  missingHelper is a permanent error
    // (no use to try and execute over and over something that's not
    // here).

    // Parameters: this has been built by our creator, from config file 
    // data. We always add the file name at the end before actual execution
    std::vector<std::string> params;
    // Filter output type. The default for ext. filters is to output html, 
    // but some don't, in which case the type is defined in the config.
    std::string cfgFilterOutputMtype;
    // Output character set if the above type is not text/html. For
    // those filters, the output charset has to be known: ie set by a command
    // line option.
    std::string cfgFilterOutputCharset; 
    bool missingHelper;
    // Resource management values
    int m_filtermaxseconds;
    int m_filtermaxmbytes;
    ////////////////

    MimeHandlerExec(RclConfig *cnf, const std::string& id);

    virtual bool next_document();
    virtual bool skip_to_document(const std::string& ipath); 

    virtual void clear_impl() override {
	m_fn.erase(); 
	m_ipath.erase();
    }

protected:
    virtual bool set_document_file_impl(const std::string& mt, 
                                        const std::string& file_path);

    std::string m_fn;
    std::string m_ipath;
    // md5 computation excluded by handler name: can't change after init
    bool m_handlernomd5;
    bool m_hnomd5init;
    // If md5 not excluded by handler name, allow/forbid depending on mime
    bool m_nomd5;
    
    // Set up the character set metadata fields and possibly transcode
    // text/plain output. 
    // @param charset when called from mh_execm, a possible explicit
    //       value from the filter (else the data will come from the config)
    virtual void handle_cs(const std::string& mt, 
                           const std::string& charset = std::string());

private:
    virtual void finaldetails();
};


// This is called periodically by ExeCmd when it is waiting for data,
// or when it does receive some. We may choose to interrupt the
// command.
class MEAdv : public ExecCmdAdvise {
public:
    MEAdv(int maxsecs = 900);
    // Reset start time to now
    void reset();
    void setmaxsecs(int maxsecs) {
        m_filtermaxseconds = maxsecs;
    }
    void newData(int n);
private:
    time_t m_start;
    int m_filtermaxseconds;
};

#endif /* _MH_EXEC_H_INCLUDED_ */
