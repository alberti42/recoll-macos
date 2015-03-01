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
#ifndef _MD5UT_H_
#define _MD5UT_H_

#include <sys/types.h>

#include "md5.h"

/** md5 utility wrappers */
#include <string>
using std::string;
extern void MD5Final(string& digest, MD5_CTX *);
extern bool MD5File(const string& filename, string& digest, string *reason);
extern string& MD5String(const string& data, string& digest);
extern string& MD5HexPrint(const string& digest, string& xdigest);
extern string& MD5HexScan(const string& xdigest, string& digest);

#endif /* _MD5UT_H_ */
