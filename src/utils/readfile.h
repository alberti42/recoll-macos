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
#ifndef _READFILE_H_INCLUDED_
#define _READFILE_H_INCLUDED_
/* @(#$Id: readfile.h,v 1.3 2007-06-02 08:30:42 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
using std::string;

/**
 * Read whole file into string. 
 * @return true for ok, false else
 */
bool file_to_string(const string &filename, string &data, string *reason = 0);

class FileScanDo {
public:
    virtual ~FileScanDo() {}
    virtual bool init(unsigned int size, string *reason) = 0;
    virtual bool data(const char *buf, int cnt, string* reason) = 0;
};
bool file_scan(const std::string &filename, FileScanDo* doer,  
	       std::string *reason = 0);

#endif /* _READFILE_H_INCLUDED_ */
