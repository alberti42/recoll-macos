/* -*- mode:c++;c-basic-offset:2 -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime-printheader.cc
 *  
 *  Description:
 *    Implementation of main mime parser components
 *  --------------------------------------------------------------------
 *  Copyright 2002-2005 Andreas Aardal Hanssen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *  --------------------------------------------------------------------
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mime.h"
#include "mime-utils.h"
#include "mime-inputsource.h"
#include "convert.h"
#include "iodevice.h"
#include "iofactory.h"

#include <string>
#include <vector>
#include <map>
#include <exception>
#include <iostream>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#ifndef NO_NAMESPACES
using namespace ::std;
#endif /* NO_NAMESPACES */

//------------------------------------------------------------------------
void Binc::MimePart::printHeader(int fd, IODevice &output,
				 vector<string> headers, bool includeheaders, 
				 unsigned int startoffset,
				 unsigned int length, string &store) const
{
  if (!mimeSource || mimeSource->getFileDescriptor() != fd) {
    delete mimeSource;
    mimeSource = new MimeInputSource(fd);
  }

  mimeSource->seek(headerstartoffsetcrlf);

  string name;
  string content;
  char cqueue[2];
  memset(cqueue, 0, sizeof(cqueue));

  bool quit = false;
  char c = '\0';

  unsigned int wrotebytes = 0;
  unsigned int processedbytes = 0;
  bool hasHeaderSeparator = false;

  while (!quit) {
    // read name
    while (1) {
      // allow EOF to end the header
      if (!mimeSource->getChar(&c)) {
	quit = true;
	break;
      }

      // assume this character is part of the header name.
      name += c;
      
      // break on the first colon
      if (c == ':')
	break;

      // break if a '\n' turned up.
      if (c == '\n') {
	// end of headers detected
	if (name == "\r\n") {
	  hasHeaderSeparator = true;
	  quit = true;
	  break;
	}
	
	// put all data back in the buffer to the beginning of this
	// line.
	for (int i = name.length(); i >= 0; --i)
	  mimeSource->ungetChar();
	
	// abort printing of header. note that in this case, the
	// headers will not end with a seperate \r\n.
	quit = true;
	name.clear();
	break;
      }
    }

    if (quit) break;

    // at this point, we have a name, that is - the start of a
    // header. we'll read until the end of the header.
    while (!quit) {
      // allow EOF to end the header.
      if (!mimeSource->getChar(&c)) {
	quit = true;
	break;
      }

      if (c == '\n') ++nlines;

      // make a fifo queue of the last 4 characters.
      cqueue[0] = cqueue[1];
      cqueue[1] = c;

      // print header
      if (cqueue[0] == '\n' && cqueue[1] != '\t' && cqueue[1] != ' ') {
	// it wasn't a space, so put it back as it is most likely
	// the start of a header name. in any case it terminates the
	// content part of this header.
	mimeSource->ungetChar();
	
	string lowername = name;
	lowercase(lowername);
	trim(lowername, ": \t");
	bool foundMatch = false;
	for (vector<string>::const_iterator i = headers.begin();
	     i != headers.end(); ++i) {
	  string nametmp = *i;
	  lowercase(nametmp);
	  if (nametmp == lowername) {
	    foundMatch = true;
	    break;
	  }
	}

	if (foundMatch == includeheaders || headers.size() == 0) {
	  string out = name + content;
	  for (string::const_iterator i = out.begin(); i != out.end(); ++i)
	    if (processedbytes >= startoffset && wrotebytes < length) {
	      if (processedbytes >= startoffset) {
		store += *i;
		++wrotebytes;
	      }
	    } else
	      ++processedbytes;
	}

	// move on to the next header
	content.clear();
	name.clear();
	break;
      }

      content += c;
    }
  }

  if (name != "") {
    string lowername = name;
    lowercase(lowername);
    trim(lowername, ": \t");
    bool foundMatch = false;
    for (vector<string>::const_iterator i = headers.begin();
	 i != headers.end(); ++i) {
      string nametmp = *i;
      lowercase(nametmp);
      if (nametmp == lowername) {
	foundMatch = true;
	break;
      }
    }
    
    if (hasHeaderSeparator || foundMatch == includeheaders || headers.size() == 0) {
      string out = name + content;
      for (string::const_iterator i = out.begin(); i != out.end(); ++i)
	if (processedbytes >= startoffset && wrotebytes < length) {
	  store += *i;
	  ++wrotebytes;
	} else
	  ++processedbytes;
    }
  }
}
