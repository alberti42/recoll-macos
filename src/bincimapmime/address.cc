/* -*- mode:c++;c-basic-offset:2 -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    address.cc
 *  
 *  Description:
 *    Implementation of the Address class.
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

#include "address.h"
#include "convert.h"
#include <string>

#ifndef NO_NAMESPACES
using namespace ::std;
using namespace Binc;
#endif /* NO_NAMESPACES */

//------------------------------------------------------------------------
Address::Address(const string &name, const string &addr)
{
  string::size_type pos = addr.find('@');
  this->name = name;
  if (pos != string::npos) {
    this->local = addr.substr(0, pos);
    this->host = addr.substr(pos + 1);
  } else this->local = addr;
}

//------------------------------------------------------------------------
Address::Address(const string &wholeaddress)
{
  string::size_type start = wholeaddress.find('<');
  string addr;
  if (start != string::npos)
    addr = wholeaddress.substr(start + 1);
  else
    addr = wholeaddress;

  trim(addr, "<>");

  if (start != string::npos)
    name = wholeaddress.substr(0, start);
  else
    name = "";
  trim(name);
  trim(name, "\"");

  start = addr.find('@');
  local = addr.substr(0, start);
  host = addr.substr(start + 1);

  trim(local);
  trim(host);
  trim(name);
}

//------------------------------------------------------------------------
string Address::toParenList(void) const
{
  string tmp = "(";
  tmp += name == "" ? "NIL" : toImapString(name);
  tmp += " NIL ";
  tmp += local == "" ? "\"\"" : toImapString(local);
  tmp += " ";
  tmp += host == "" ? "\"\"" : toImapString(host);
  tmp += ")";

  return tmp;
}
