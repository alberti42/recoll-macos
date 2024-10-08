/* Copyright (C) 2004-2023 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _UNACPP_H_INCLUDED_
#define _UNACPP_H_INCLUDED_

#include <string>

// All the following function take an UTF-8 input and output UTF-8

// A small stringified wrapper for unac.c
enum UnacOp {UNACOP_UNAC = 1, UNACOP_FOLD = 2, UNACOP_UNACFOLD = 3};
extern bool unacmaybefold(const std::string& in, std::string& out, UnacOp what);

// Utility function to determine if string begins with capital
extern bool unaciscapital(const std::string& in);
// Utility function to determine if string has upper-case anywhere
extern bool unachasuppercase(const std::string& in);
// Utility function to determine if any character is accented. This
// appropriately ignores the characters from unac_except_chars which
// are really separate letters
extern bool unachasaccents(const std::string& in);

// Simplified interface to unacmaybefold for cases where we just mimic tolower()
extern std::string unactolower(const std::string& in);

#endif /* _UNACPP_H_INCLUDED_ */
