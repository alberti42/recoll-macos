/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime-getpart.cc
 *  
 *  Description:
 *    Implementation of main mime parser components
 *  --------------------------------------------------------------------
 *  Copyright 2002-2004 Andreas Aardal Hanssen
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
#include "convert.h"
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <iostream>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

using namespace ::std;

//------------------------------------------------------------------------
const Binc::MimePart *Binc::MimePart::getPart(const string &findpart,
					      string genpart, FetchType fetchType) const
{
  if (findpart == genpart)
    return this;

  if (isMultipart()) {
    if (members.size() != 0) {
      vector<MimePart>::const_iterator i = members.begin();
      int part = 1;
      while (i != members.end()) {
	BincStream ss;

	ss << genpart;
	if (genpart != "")
	  ss << ".";
	ss << part;
	
	const MimePart *m;
	if ((m = (*i).getPart(findpart, ss.str())) != 0) {
	  if (fetchType == FetchHeader && m->isMessageRFC822())
	    m = &m->members[0];

	  return m;
	}

	++i;
	++part;
      }
    }
  } else if (isMessageRFC822()) {
    if (members.size() == 1) {
      const MimePart *m = members[0].getPart(findpart, genpart);
      return m;
    } else {
      return 0;
    }
  } else {
    // Singlepart
    if (genpart != "")
      genpart += ".";
    genpart += "1";

    if (findpart == genpart)
      return this;
  }

  return 0;
}
