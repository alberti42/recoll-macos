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
#ifndef _BASE64_H_INCLUDED_
#define _BASE64_H_INCLUDED_
/* @(#$Id: base64.h,v 1.3 2008-07-28 08:42:52 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

void base64_encode(const std::string& in, std::string& out);
bool base64_decode(const std::string& in, std::string& out);

#endif /* _BASE64_H_INCLUDED_ */
