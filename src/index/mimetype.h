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
#ifndef _MIMETYPE_H_INCLUDED_
#define _MIMETYPE_H_INCLUDED_
/* @(#$Id: mimetype.h,v 1.5 2006-01-30 11:15:27 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

class RclConfig;

/**
 * Try to determine a mime type for filename. 
 * This may imply more than matching the suffix, the name must be usable
 * to actually access file data.
 */
string mimetype(const std::string &filename, RclConfig *cfg, bool usfc);


#endif /* _MIMETYPE_H_INCLUDED_ */
