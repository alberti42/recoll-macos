/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    mime.cc
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

#ifndef NO_NAMESPACES
using namespace ::std;
#endif /* NO_NAMESPACES */

//------------------------------------------------------------------------
Binc::MimeDocument::MimeDocument(void) : MimePart()
{
  allIsParsed = false;
  headerIsParsed = false;
}

//------------------------------------------------------------------------
Binc::MimeDocument::~MimeDocument(void)
{
}

//------------------------------------------------------------------------
void Binc::MimeDocument::clear(void) const
{
  members.clear();
  h.clear();
  headerIsParsed = false;
  allIsParsed = false;
}

//------------------------------------------------------------------------
void Binc::MimePart::clear(void) const
{
  members.clear();
  h.clear();
}

//------------------------------------------------------------------------
Binc::MimePart::MimePart(void)
{
  size = 0;
  messagerfc822 = false;
  multipart = false;

  nlines = 0;
  nbodylines = 0;
}

//------------------------------------------------------------------------
Binc::MimePart::~MimePart(void)
{
}

//------------------------------------------------------------------------
Binc::HeaderItem::HeaderItem(void)
{
}

//------------------------------------------------------------------------
Binc::HeaderItem::HeaderItem(const string &key, const string &value)
{
  this->key = key;
  this->value = value;
}

//------------------------------------------------------------------------
Binc::Header::Header(void)
{
}

//------------------------------------------------------------------------
Binc::Header::~Header(void)
{
}

//------------------------------------------------------------------------
bool Binc::Header::getFirstHeader(const string &key, HeaderItem &dest) const
{
  string k = key;
  lowercase(k);

  for (vector<HeaderItem>::const_iterator i = content.begin();
       i != content.end(); ++i) {
    string tmp = (*i).getKey();
    lowercase(tmp);

    if (tmp == k) {
      dest = *i;
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------
bool Binc::Header::getAllHeaders(const string &key, vector<HeaderItem> &dest) const
{
  string k = key;
  lowercase(k);

  for (vector<HeaderItem>::const_iterator i = content.begin();
       i != content.end(); ++i) {
    string tmp = (*i).getKey();
    lowercase(tmp);
    if (tmp == k)
      dest.push_back(*i);
  }

  return (dest.size() != 0);
}

//------------------------------------------------------------------------
void Binc::Header::clear(void) const
{
  content.clear();
}

//------------------------------------------------------------------------
void Binc::Header::add(const string &key, const string &value)
{
  content.push_back(HeaderItem(key, value));
}
